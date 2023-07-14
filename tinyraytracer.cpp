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
    Material(float refract_index,float kd,float ks,float kr,float krefra,const Vec3f& color,float specular_exponent) :refract_index(refract_index),kd(kd),ks(ks),kr(kr),krefra(krefra), diffuse_color(color), specular_exponent(specular_exponent) {}
    Material() : diffuse_color() {}
    Vec3f diffuse_color;
    float refract_index;
    float kd;//diffuse系数
    float ks;//specular系数
    float kr;//反射系数
    float krefra;
    float specular_exponent;
};
Material      ivory(1.0, 0.6, 0.3, 0.1, 0.0, Vec3f(0.4, 0.4, 0.3), 50.);
Material      glass(1.5, 0.0, 0.5, 0.1, 0.8, Vec3f(0.6, 0.7, 0.8), 125.);
Material red_rubber(1.0, 0.9, 0.1, 0.0, 0.0, Vec3f(0.3, 0.1, 0.1), 10.);
Material     mirror(1.0, 0.0, 10.0, 0.8, 0.0, Vec3f(1.0, 1.0, 1.0), 1425.);
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
    //从orig位置射出dir方向的线是否与该sphere相交，返回N为接触点的法向向量,dist为orig到接触点的距离
    bool raySphere_intersect(Vec3f orig,const Vec3f dir,Vec3f& N,float& dist)
    {
        Vec3f view = dir;
        Vec3f pc = center-orig;
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
                N = orig+(view * dist)-center;
                N.normalize();
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
Vec3f refract(const Vec3f I, const Vec3f N, const float& refractive_index) { // Snell's law
    float cosi = -std::max(-1.f, std::min(1.f, I * N));
    float etai = 1, etat = refractive_index;
    Vec3f n = N;
    if (cosi < 0) { // if the ray is inside the object, swap the indices and invert the normal to get the correct result
        cosi = -cosi;
        std::swap(etai, etat); n = -N;
    }
    float eta = etai / etat;
    float k = 1 - eta * eta * (1 - cosi * cosi);
    return k < 0 ? Vec3f(0, 0, 0) : I * eta + n * (eta * cosi - sqrtf(k));
}
Vec3f refract1( Vec3f dir, Vec3f N,const float& refract_index)
{  
    float costheta1 = N * (-dir);
    float sintheta1 = sqrt(1 - costheta1 * costheta1);
    float sintheta2 = sintheta1/refract_index;
    if (costheta1 < 0)
    {
        costheta1 = -costheta1;
        sintheta2 = sintheta1 * refract_index;
        N = -N;
        if (sintheta2 > 1)
        {
            return Vec3f(0,0,0);
        }
    }
    float costheta2 = sqrt(1 - sintheta2 * sintheta2);
    return dir * (sintheta2 / sintheta1) - N * (costheta2 - costheta1 * sintheta2 / sintheta1);
}
void writeFile()
{
    std::ofstream ofs;
    ofs.open("./outShadow.ppm", std::ofstream::out | std::ofstream::binary);
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
bool scene_intersect(Vec3f orig, Vec3f dir, std::vector<Sphere>& spheres,Vec3f&hit , Vec3f& N, float& dist , Material& material)
{
    dir.normalize();
    int sz_spheres = spheres.size();
    int f = 0;
    for (int i = 0; i < sz_spheres; ++i)
    {
        if (spheres[i].raySphere_intersect(orig, dir, N, dist))
        {
            f = 1;
            hit = orig + (dir * dist);
            material = spheres[i].material;
        }
    }

    if (fabs(dir.y)>1e-3)
    {
        float d=-(orig.y + 4) / dir.y;
        Vec3f ck_hit = orig + dir * d;
        if (d>(1e-3)&&d<dist&&fabs(ck_hit.x) < 10 && ck_hit.z<-10 && ck_hit.z>-30)
        {
            f = 1;
            hit = ck_hit;
            dist= d;
            N = Vec3f(0,1,0);
            material = ivory;
            material.diffuse_color = (int(.5 * hit.x + 1000) + int(.5 * hit.z)) & 1 ? Vec3f(.3, .3, .3) : Vec3f(.3, .2, .1);
        }
    }
    if (f)return true;
    return false;
}
Vec3f castRay(Vec3f orig, Vec3f dir, std::vector<Sphere>& spheres, std::vector<Light>& lights, int depth)
{
    int spheres_sz = spheres.size();
    float dist = 100000;
    Vec3f N,hitPoint;//接触点的法向量
    Material material;
    

    if (depth>4||!scene_intersect(orig, dir, spheres, hitPoint, N, dist, material))
    {
        return Vec3f(0.2, 0.7, 0.8);
    }
    else
    {
        int lights_sz = lights.size();
        float diffuse_intensity = 0;
        float specular_intensity1 = 0;
        float specular_intensity2 = 0;

        N.normalize();
        dir.normalize();
        Vec3f refract_dir = refract(dir, N, material.refract_index).normalize();
        Vec3f refract_orig = refract_dir * N < 0 ? hitPoint - N * 1e-3 : hitPoint + N * 1e-3; // offset the original point to avoid occlusion by the object itself
        Vec3f refract_color = castRay(refract_orig, refract_dir, spheres, lights, depth + 1);
        Vec3f reflect_dir = reflect(dir, N).normalize();
        Vec3f reflect_orig = reflect_dir * N < 0 ? hitPoint - N * 1e-3 : hitPoint + N * 1e-3; // offset the original point to avoid occlusion by the object itself
        Vec3f reflect_color = castRay(reflect_orig, reflect_dir, spheres, lights, depth + 1);

        #pragma omp parallel for
        for (int k = 0; k < lights_sz; ++k)
        {
            Vec3f light_dir = lights[k].position - hitPoint;
            light_dir.normalize();
            if (N * light_dir <= 0)continue;
            Material none_m = ivory;
            Vec3f none_N;
            float PLdist = (lights[k].position - hitPoint).norm();
            Vec3f shadow_orig = hitPoint;
            Vec3f shadow_hp;
            if (scene_intersect(shadow_orig, light_dir, spheres, shadow_hp, none_N, 
                PLdist, none_m) && PLdist < (lights[k].position - hitPoint).norm())
            {
                continue;
            }
            Vec3f h = -dir + light_dir;
            h.normalize();
            float ambient = 0;
            diffuse_intensity += lights[k].intensity * std::max(0.f, light_dir * N);
            float t1 = h * N;
            float t2 = reflect(-light_dir, N) * (-dir);
            specular_intensity1 += lights[k].intensity * powf(std::max(0.f, t1), material.specular_exponent);
            specular_intensity2 += lights[k].intensity * powf(std::max(0.f, t2), material.specular_exponent);
        }
        //return refract_color * material.krefra;
        return material.diffuse_color * diffuse_intensity * material.kd + Vec3f(1., 1., 1.) * specular_intensity2 * material.ks+reflect_color*material.kr+refract_color*material.krefra;
    }
}
void render(std::vector<Sphere>& spheres, std::vector<Light>&lights)
{
    framebuffer.resize(width * height);
    for (size_t j = 0; j < height; j++) 
    {
        for (size_t i = 0; i < width; i++) 
        {
            float screen_width = 2 * tan(fov / 2)*aspect_ratio;
            float screen_height = 2 * tan(fov / 2);
            float x = ((i + 0.5) / width) *screen_width-screen_width/2;
            float y = -(((j + 0.5) / height) * screen_height - screen_height / 2);
            Vec3f view_direction(x,y,-1);
            view_direction.normalize();
            framebuffer[j * width + i] = castRay(Vec3f(0, 0, 0), view_direction, spheres, lights,0);
        }
    }
    writeFile();
}

int main() 
{
    std::vector<Sphere> spheres;
    spheres.push_back(Sphere(Vec3f(-3, 0, -16), 2, ivory));
    spheres.push_back(Sphere(Vec3f(-1.0, -1.5, -12), 2, glass));
    spheres.push_back(Sphere(Vec3f(1.5, -0.5, -18), 3, red_rubber));
    spheres.push_back(Sphere(Vec3f(7, 5, -18), 4, mirror));

    std::vector<Light>  lights;
    lights.push_back(Light(Vec3f(-20, 20, 20), 1.5));
    lights.push_back(Light(Vec3f(30, 50, -25), 1.8));
    lights.push_back(Light(Vec3f(30, 20, 30), 1.7));

    render(spheres, lights);

    return 0;
}

