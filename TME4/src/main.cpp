// ImageResizerApp.cpp
#include <QCoreApplication> // For non-GUI Qt app

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>

#include "util/CLI11.hpp" // Header only lib for argument parsing

#include "BoundedBlockingQueue.h"
#include "Tasks.h"
#include "util/ImageUtils.h"
#include "util/processRSS.h"
#include "util/thread_timer.h"

struct Options {
  std::filesystem::path inputFolder = "input_images/";
  std::filesystem::path outputFolder = "output_images/";
  std::string mode = "resize";
  int num_threads = 4;
  int nbread = 1;
  int nbsaver = 1;
  int nbresizer = 1;

  friend std::ostream &operator<<(std::ostream &os, const Options &opts) {
    os << "input folder '" << opts.inputFolder.string() << "', output folder '"
       << opts.outputFolder.string() << "', mode '" << opts.mode << "', nthreads "
       << opts.num_threads;
    return os;
  }
};

int parseOptions(int argc, char *argv[], Options &opts);

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv); // Initialize Qt for image format plugins

  Options opts;
  // rely on CLI11 to parse command line options
  // see implementation of parseOptions below main function
  int code = parseOptions(argc, argv, opts);
  if (code != 0) {
    return code;
  }

  std::cout << "Image resizer starting with " << opts << std::endl;

  auto start_time = std::chrono::steady_clock::now();
  pr::thread_timer main_timer;

  if (opts.mode == "resize") {
    // Single-threaded: direct load/resize/save in callback
    pr::findImageFiles(opts.inputFolder, [&](const std::filesystem::path &file) {
      QImage original = pr::loadImage(file);
      if (!original.isNull()) {
        QImage resized = pr::resizeImage(original);
        std::filesystem::path outputFile = opts.outputFolder / file.filename();
        pr::saveImage(resized, outputFile);
      }
    });
  } else if (opts.mode == "pipe_mt") {
    pr::FileQueue fileQueue(10);

    // 2. Start the worker thread
    std::vector<std::thread> workers;
    for (size_t i = 0; i < opts.num_threads; i++) {
      workers.emplace_back(pr::treatImage, std::ref(fileQueue), std::ref(opts.outputFolder));
    }

    // 3. Populate file queue synchronously
    pr::findImageFiles(opts.inputFolder,
                       [&](const std::filesystem::path &file) { fileQueue.push(file); });

    // 4. Push poison pill
    for (size_t i = 0; i < opts.num_threads; i++) {
      fileQueue.push(pr::FILE_POISON);
    }

    // 5. Join the worker thread
    for (auto &worker : workers) {
      worker.join();
    }
  } else if (opts.mode == "pipe") {
    // 1. Single-threaded pipeline: file discovery -> treatImage (load/resize/save)
    pr::FileQueue fileQueue(10);

    // 2. Start the worker thread
    std::thread worker(pr::treatImage, std::ref(fileQueue), std::ref(opts.outputFolder));

    // 3. Populate file queue synchronously
    pr::findImageFiles(opts.inputFolder,
                       [&](const std::filesystem::path &file) { fileQueue.push(file); });

    // 4. Push poison pill
    fileQueue.push(pr::FILE_POISON);

    // 5. Join the worker thread
    worker.join();
  } else if (opts.mode == "mt_pipeline") {
    // 1. Single-threaded pipeline: file discovery -> treatImage (load/resize/save)
    pr::FileQueue fileQueue(10);
    pr::ImageTaskQueue imageTaskQueue(10);
    pr::ImageTaskQueue resised_TaskQueue(10);

    // 2. Start the worker thread
    std::vector<std::thread> readers;
    for (int i = 0; i < opts.nbread; i++) {
      readers.emplace_back(pr::reader, std::ref(fileQueue), std::ref(imageTaskQueue));
    }
    std::vector<std::thread> resizers;
    for (int i = 0; i < opts.nbresizer; i++) {
      resizers.emplace_back(pr::resizer, std::ref(imageTaskQueue), std::ref(resised_TaskQueue));
    }
    std::vector<std::thread> savers;
    for (int i = 0; i < opts.nbsaver; i++) {
      savers.emplace_back(pr::saver, std::ref(resised_TaskQueue), std::ref(opts.outputFolder));
    }

    // 3. Populate file queue synchronously
    pr::findImageFiles(opts.inputFolder,
                       [&](const std::filesystem::path &file) { fileQueue.push(file); });

    // 4. Push poison pill

    for (int i = 0; i < opts.nbread; i++) {
      fileQueue.push(pr::FILE_POISON);
    }
    // la prochaine methode ou plutot  celle noraml attendre la fin de chaque etapes puis push les poison
    // 5. Join the worker threads stage by stage, inserting the right number of poison pills
    /*for (auto &worker : readers) {
      worker.join();
    }
    if (opts.nbresizer > opts.nbread) {
      int nb = opts.nbresizer - opts.nbread;
      for (int i = 0; i < nb; i++) {
        imageTaskQueue.push(pr::TASK_POISON);
      }
    }
    for (auto &worker : resizers) {
      worker.join();
    }
    for (int i = 0; i < opts.nbsaver; ++i) {
      resised_TaskQueue.push(pr::TASK_POISON);
    }
    for (auto &worker : savers) {
      worker.join();
    }*/
   /*ou attedndre juste les readers on suite on laisser les residers pop au mm temps le poison mais pas ouf  pb marche pas avec 1 32 1 par exemple je sais pas pk*/
  }

  else {
    std::cerr << "Unknown mode '" << opts.mode << "'. Supported modes: resize, pipe" << std::endl;
    return 1;
  }

  std::stringstream ss;
  ss << "Thread " << std::this_thread::get_id() << " (main): " << main_timer << " ms CPU"
     << std::endl;
  std::cout << ss.str();

  auto end = std::chrono::steady_clock::now();
  std::cout << "Total runtime (wall clock): "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end - start_time).count()
            << " ms" << std::endl;

  // Report memory usage at the end
  std::cout << "Memory usage: " << process::getResidentMemory() << std::endl;

  // Report total CPU time across all timers
  std::cout << "Total CPU time across all threads: " << pr::thread_timer::getTotalCpuTimeMs()
            << " ms" << std::endl;

  return 0;
}

int parseOptions(int argc, char *argv[], Options &opts) {
  Options default_opts; // Use defaults from struct for CLI11 help
  CLI::App cli_app(
      "Image Resizer Application. Scales down images in input folder, writes to output folder.");

  cli_app.add_option("-i,--input", opts.inputFolder, "Input folder containing images")
      ->check(CLI::ExistingDirectory)
      ->default_str(default_opts.inputFolder.string());

  cli_app
      .add_option("-o,--output", opts.outputFolder,
                  "Output folder for resized images (will be created if needed)")
      ->default_str(default_opts.outputFolder.string());

  cli_app.add_option("-m,--mode", opts.mode, "Processing mode")
      ->check(CLI::IsMember({"resize", "pipe", "pipe_mt", "mt_pipeline"})) // TODO : add modes
      ->default_str(default_opts.mode);

  cli_app.add_option("-n,--nthreads", opts.num_threads, "Number of threads")
      ->check(CLI::PositiveNumber)
      ->default_val(default_opts.num_threads);

  cli_app.add_option("-r,--nbread", opts.nbread, "Number of read")
      ->check(CLI::PositiveNumber)
      ->default_val(default_opts.nbread);

  cli_app.add_option("-s,--nbresize", opts.nbresizer, "Number of resizer")
      ->check(CLI::PositiveNumber)
      ->default_val(default_opts.nbresizer);
  cli_app.add_option("-w,--nbwriter", opts.nbsaver, "Number of saver")
      ->check(CLI::PositiveNumber)
      ->default_val(default_opts.nbsaver);
  try {
    cli_app.parse(argc, argv);
  } catch (const CLI::CallForHelp &e) {
    cli_app.exit(e);
    std::exit(0);
  } catch (const CLI::ParseError &e) {
    return cli_app.exit(e);
  }

  if (!std::filesystem::exists(opts.outputFolder)) {
    if (!std::filesystem::create_directories(opts.outputFolder)) {
      std::cerr << "Failed to create the output folder." << std::endl;
      return 1;
    }
  }

  return 0;
}
