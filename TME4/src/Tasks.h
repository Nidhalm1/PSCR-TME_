#ifndef TASKS_H
#define TASKS_H

#include <QImage>
#include <filesystem>
#include <utility>
#include "BoundedBlockingQueue.h"

namespace pr {

using FileQueue = BoundedBlockingQueue<std::filesystem::path>;

const std::filesystem::path FILE_POISON{};



// load/resize/save 
void treatImage(FileQueue& fileQueue, const std::filesystem::path& outputFolder);


struct TaskData {
    QImage image;
    std::filesystem::path file;
    TaskData() = default ;
    TaskData(const QImage& image, const std::filesystem::path& file)
        : image(image), file(file) {}
};
//using ImageTaskQueue = BoundedBlockingQueue<TaskData*>;
// or
using ImageTaskQueue = BoundedBlockingQueue<TaskData>;


const TaskData TASK_POISON = TaskData();
inline bool operator==(const TaskData& a, const TaskData& b) {
    // On peut simplement comparer les chemins (le poison a un chemin vide)
    return a.file == b.file;
}


// TODO
void reader(FileQueue& fileQueue, ImageTaskQueue& imageQueue);
void resizer(ImageTaskQueue& imageQueue, ImageTaskQueue& resizedQueue);
void saver(ImageTaskQueue& resizedQueue, const std::filesystem::path& outputFolder);

} // namespace pr

#endif // TASKS_H
