#pragma once

#include "Image.h"
#include "Pool.h"
#include "Ray.h"
#include "Scene.h"
#include "PixelJob.h"


#include <thread>

namespace pr {

// Classe pour rendre une scène dans une image
class Renderer {
public:
  // Rend la scène dans l'image
  void render(const Scene &scene, Image &img) {
    // les points de l'ecran, en coordonnées 3D, au sein de la Scene.
    // on tire un rayon de l'observateur vers chacun de ces points
    const Scene::screen_t &screen = scene.getScreenPoints();

    // pour chaque pixel, calculer sa couleur
    for (int x = 0; x < scene.getWidth(); x++) {
      for (int y = 0; y < scene.getHeight(); y++) {
        // le point de l'ecran par lequel passe ce rayon
        auto &screenPoint = screen[y][x];
        // le rayon a inspecter
        Ray ray(scene.getCameraPos(), screenPoint);

        int targetSphere = scene.findClosestInter(ray);

        if (targetSphere == -1) {
          // keep background color
          continue;
        } else {
          const Sphere &obj = scene.getObject(targetSphere);
          // pixel prend la couleur de l'objet
          Color finalcolor = scene.computeColor(obj, ray);
          // mettre a jour la couleur du pixel dans l'image finale.
          img.pixel(x, y) = finalcolor;
        }
      }
    }
  }
  void renderThreadPerPixel(const Scene &scene, Image &img) {
    const Scene::screen_t &screen = scene.getScreenPoints();
    std::vector<std::thread> threads;
    for (int x = 0; x < scene.getWidth(); x++) {
      for (int y = 0; y < scene.getHeight(); y++) {
        threads.emplace_back([&, x, y] { // le point de l'ecran par lequel passe ce rayon
          auto &screenPoint = screen[y][x];
          // le rayon a inspecter
          Ray ray(scene.getCameraPos(), screenPoint);

          int targetSphere = scene.findClosestInter(ray);

          if (targetSphere == -1) {
            // keep background color
            return;
          } else {
            const Sphere &obj = scene.getObject(targetSphere);
            // pixel prend la couleur de l'objet
            Color finalcolor = scene.computeColor(obj, ray);
            // mettre a jour la couleur du pixel dans l'image finale.
            img.pixel(x, y) = finalcolor;
          }
        });
      }
    }
    for (auto &t : threads)
      t.join();
  }
  void renderThreadPerRow(const Scene &scene, Image &img) {
    const Scene::screen_t &screen = scene.getScreenPoints();
    std::vector<std::thread> threads;
    for (int y = 0; y < scene.getHeight(); y++) {
      threads.emplace_back([&, y] { // le point de l'ecran par lequel passe ce rayon
        for (int x = 0; x < scene.getWidth(); x++) {
          auto &screenPoint = screen[y][x];
          // le rayon a inspecter
          Ray ray(scene.getCameraPos(), screenPoint);

          int targetSphere = scene.findClosestInter(ray);
          if (targetSphere == -1) {
            // keep background color
            continue;
          } else {
            const Sphere &obj = scene.getObject(targetSphere);
            // pixel prend la couleur de l'objet
            Color finalcolor = scene.computeColor(obj, ray);
            // mettre a jour la couleur du pixel dans l'image finale.
            img.pixel(x, y) = finalcolor;
          }
        }
      });
    }
    for (auto &t : threads)
      t.join();
  }
  void renderThreadManual(const Scene &scene, Image &img, int nbthread) {
    const Scene::screen_t &screen = scene.getScreenPoints();
    std::vector<std::thread> threads;
    int h = scene.getHeight();
    int nbT = std::max(1, std::min(nbthread, h));
    int take = h / nbT;
    int reste = h % nbT;
    int debut = 0;
    int fin = 0;
    for (int i = 0; i < nbT; i++) {
      fin = debut + take + (i < reste ? 1 : 0);
      threads.emplace_back([&, debut, fin] {
        for (int y = debut; y < fin; y++) {
          for (int x = 0; x < scene.getWidth(); x++) {
            // le point de l'ecran par lequel passe ce rayon
            auto &screenPoint = screen[y][x];
            // le rayon a inspecter
            Ray ray(scene.getCameraPos(), screenPoint);
            int targetSphere = scene.findClosestInter(ray);
            if (targetSphere == -1) {
              // keep background color
              continue;
            } else {
              const Sphere &obj = scene.getObject(targetSphere);
              // pixel prend la couleur de l'objet
              Color finalcolor = scene.computeColor(obj, ray);
              // mettre a jour la couleur du pixel dans l'image finale.
              img.pixel(x, y) = finalcolor;
            }
          }
        };
      });
      debut = fin;
    }
    for (auto &t : threads)
      t.join();
  }
  void renderPoolPixel(const Scene &scene, Image &img, int nbthread) {
    Pool p(254);
    p.start(nbthread);
    const Scene::screen_t &screen = scene.getScreenPoints();
    for (int x = 0; x < scene.getWidth(); x++) {
      for (int y = 0; y < scene.getHeight(); y++) {
        // le point de l'ecran par lequel passe ce rayon
       PixelJob * pixel = new PixelJob(x,y,img,screen,scene);
       p.submit(pixel);
      }
    }
    p.stop();
  }
};

} // namespace pr