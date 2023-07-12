#include <limits>
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include "geometry.h"
const int width = 1024;
const int height = 768;
const float fov = 3.14 / 2;    
const float aspect_ratio = 1.0*width / height;
std::vector<Vec3f> framebuffer;
class Sphere     
{
public:
    Vec3f center;
    float radius;
    Sphere(const Vec3f& center,const float& radius)
    {
        this->center = center;
        this->radius = radius;
    }
    bool raySphere_intersect(const Vec3f& lightPos,const Vec3f& lightDir)
    {
        Vec3f light=lightDir - lightPos;
        Vec3f pc = center - lightPos;
        float cosTheta = pc * light / pc.norm() / light.norm();
        if (cosTheta < 0)
        {
            return false;
        }
        float pdLen = pc.norm() * cosTheta;
        float dcLen = (pc.norm()*pc.norm() - pdLen * pdLen);
        if (dcLen <= 0)return true;
        dcLen = sqrt(dcLen);
        if (dcLen <= radius)return true;
        return false;
    }
};

void writeFile()
{
    std::ofstream ofs;
    ofs.open("./out.ppm", std::ofstream::out | std::ofstream::binary);
    ofs << "P6\n" << width << " " << height << "\n255\n";
    for (size_t i = 0; i < height * width; ++i) {
        for (size_t j = 0; j < 3; j++) {
            ofs << (char)(255 * std::max(0.f, std::min(1.f, framebuffer[i][j])));
        }
    }
    ofs.close();
}
void render(Sphere& sphere) 
{
    framebuffer.resize(width * height);
    for (size_t j = 0; j < height; j++) {
        for (size_t i = 0; i < width; i++) {
            float screen_width = 2 * tan(fov / 2);
            float screen_height = screen_width / aspect_ratio;
            float x = ((i + 0.5) / width) *screen_width-screen_width/2;
            float y = ((j + 0.5) / height) * screen_height - screen_height / 2;
            //printf("%f %f\n", x, y);
            Vec3f light_position(0, 0, 0);
            Vec3f light_dir(x, y, -1);
            if (sphere.raySphere_intersect(light_position, light_dir))
            {
                framebuffer[j * width + i] = Vec3f(0.4, 0.4, 0.3);
            }
            else 
            {
                framebuffer[j * width + i] = Vec3f(0.2, 0.7, 0.8);
            }
        }
    }
    writeFile();
}

int main() {
    Sphere sphere(Vec3f(-3, 0, -16), 2);
    render(sphere);
    return 0;
}

