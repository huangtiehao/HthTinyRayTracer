#include <limits>
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include "geometry.h"
const int width = 1024;
const int height = 768;
const float fov =  1;    
const float aspect_ratio = 1.0*width / height;
std::vector<Vec3f> framebuffer;
class Light
{
public:
    Light(const Vec3f& position,const float& intensity)
    {
        this->position = position;
        this->intensity = intensity;
    }
    Vec3f position;
    float intensity;
};
class Material 
{
public:
    Material(float kd,float ks,const Vec3f& color,float specular_exponent) :kd(kd),ks(ks),diffuse_color(color),specular_exponent(specular_exponent) {}
    Material() : diffuse_color() {}
    Vec3f diffuse_color;
    float kd;
    float ks;
    float specular_exponent;
};
class Sphere     
{
public:
    Vec3f center;
    float radius;
    Material material;
    Sphere(const Vec3f& center,const float& radius,Material& material)
    {
        this->center = center;
        this->radius = radius;
        this->material = material;
    }
    bool raySphere_intersect(const Vec3f& viewDir,Vec3f& N,float& dist)
    {
        Vec3f view = viewDir;
        Vec3f pc = center;
        float cosTheta = pc * view / pc.norm()/view.norm();
        if (cosTheta < 0)
        {
            return false;
        }
        float pdLen = pc.norm() * cosTheta;
        float dcLen = (pc.norm() * pc.norm() - pdLen * pdLen);
        dcLen = sqrt(dcLen);

        if (dcLen <= radius)
        {
            if (pdLen - sqrt(radius * radius - dcLen * dcLen) < dist)
            {
                dist = pdLen - sqrt(radius * radius - dcLen);
                N = view * dist-center;
                return true;
            }
            else return false;
        }
        return false;
    }
};
Vec3f reflect(const Vec3f& I, const Vec3f& N) {
    return I - N * 2.f * (I * N);
}
void writeFile()
{
    std::ofstream ofs;
    ofs.open("./out.ppm", std::ofstream::out | std::ofstream::binary);
    ofs << "P6\n" << width << " " << height << "\n255\n";
    for (size_t i = 0; i < height * width; ++i) {
        Vec3f& c = framebuffer[i];
        float max = std::max(c[0], std::max(c[1], c[2]));
        if (max > 1) c = c * (1. / max);
        for (size_t j = 0; j < 3; j++) {
            ofs << (char)(255 * std::max(0.f, std::min(1.f, framebuffer[i][j])));
        }
    }
    ofs.close();
}
void render(std::vector<Sphere>& spheres, std::vector<Light>&lights)
{
    framebuffer.resize(width * height);
    for (size_t j = 0; j < height; j++) {
        for (size_t i = 0; i < width; i++) {
            float screen_width = 2 * tan(fov / 2)*aspect_ratio;
            float screen_height = 2 * tan(fov / 2);
            float x = ((i + 0.5) / width) *screen_width-screen_width/2;
            float y = -(((j + 0.5) / height) * screen_height - screen_height / 2);
            int spheres_sz = spheres.size();
            Vec3f view_direction(x,y,-1);
            view_direction.normalize();
            float dist=10000000;
            Vec3f N;
            Material material;
            for (int k = 0; k < spheres_sz; ++k)
            {
                if (spheres[k].raySphere_intersect(view_direction,N,dist))
                {   
                    material = spheres[k].material;
                }
            }
            if (dist == 10000000)
            {
                framebuffer[j * width + i] = Vec3f(0.2,0.7,0.8);
            }
            else
            {
                int lights_sz = lights.size();
                float diffuse_intensity=0;
                float specular_intensity1=0;
                float specular_intensity2 = 0;
                for (int k = 0; k < lights_sz; ++k)
                {
                    Vec3f light_dir = lights[k].position -  view_direction*dist;

                    light_dir.normalize();
                    N.normalize();
                    Vec3f h = -view_direction + light_dir;
                    h.normalize();
                    float ambient = 0;
                    diffuse_intensity += lights[k].intensity * std::max(0.f, light_dir * N);
                    float t1 = h*N;
                    float t2 = reflect(-light_dir, N) * ( - view_direction);
                    specular_intensity1 += lights[k].intensity*powf(std::max(0.f,t1), material.specular_exponent);
                    specular_intensity2 += lights[k].intensity*powf(std::max(0.f,t2), material.specular_exponent);
                    //printf("%f %f\n",diffuse_intensity,specular_intensity);
                }
                framebuffer[j * width + i] = material.diffuse_color* diffuse_intensity*material.kd+Vec3f(1.,1.,1.)*specular_intensity2*material.ks;
            }
        }
    }
    writeFile();
}

int main() 
{
    Material      ivory(0.6,0.3,Vec3f(0.4, 0.4, 0.3),50);
    Material red_rubber(0.9,0.1,Vec3f(0.3, 0.1, 0.1),10);

    std::vector<Sphere> spheres;
    spheres.push_back(Sphere(Vec3f(-3, 0, -16), 2, ivory));
    spheres.push_back(Sphere(Vec3f(-1.0, -1.5, -12), 2, red_rubber));
    spheres.push_back(Sphere(Vec3f(1.5, -0.5, -18), 3, red_rubber));
    spheres.push_back(Sphere(Vec3f(7, 5, -18), 4, ivory));

    std::vector<Light>  lights;
    lights.push_back(Light(Vec3f(-20, 20, 20), 1.5));
    lights.push_back(Light(Vec3f(30, 50, -25), 1.8));
    lights.push_back(Light(Vec3f(30, 20, 30), 1.7));

    render(spheres, lights);

    return 0;
}

