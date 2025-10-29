#include "Job.h"
#include "Image.h" // Make sure Image.h defines the Image class
#include "Pool.h"
#include "Ray.h"
#include "Scene.h"
#include "Sphere.h"
#include "Color.h" // Add this if Color is defined in Color.h

namespace pr {
class PixelJob : public pr::Job {
private:
  /* data */
  int x;
  int y;
  pr::Image &img;
  const pr::Scene::screen_t &screen;
  const pr::Scene &scene;
public:
  PixelJob(int x, int y, pr::Image &img, const pr::Scene::screen_t &screen,const pr::Scene &scene) : x(x), y(y), img(img), screen(screen) ,scene(scene) {} // passer le screen?);

  void run() {
    // le point de l'ecran par lequel passe ce rayon
    auto &screenPoint = screen[y][x];
    // le rayon a inspecter
    pr::Ray ray(scene.getCameraPos(), screenPoint);
    int targetSphere = scene.findClosestInter(ray);

    if (targetSphere == -1) {
      // keep background color
      return;
    } else {
      const pr::Sphere &obj = scene.getObject(targetSphere);
      pr::Color finalcolor = scene.computeColor(obj, ray);
      // mettre a jour la couleur du pixel dans l'image finale.
      img.pixel(x, y) = finalcolor;
    }
  }
  ~PixelJob() override = default;;

};
}


