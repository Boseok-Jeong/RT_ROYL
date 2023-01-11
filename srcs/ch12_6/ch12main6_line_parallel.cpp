#include "../../lib/v3/rtweekend.h"

#include "../../lib/v3/aarect.h"
#include "../../lib/v3/box.h"
#include "../../lib/v3/camera.h"
#include "../../lib/v3/color.h"
#include "../../lib/v3/hittable_list.h"
#include "../../lib/v3/material.h"
#include "../../lib/v3/sphere.h"

#include <omp.h>

#include <iostream>
color ray_color(
	const ray& r,
	const color& background,
	const hittable& world,
	shared_ptr<hittable> lights,
	int depth
) {
	hit_record rec;

	// If we've exceeded the ray bounce limit, no more light is gathered
	if (depth <= 0)
		return color(0, 0, 0);

	// If the ray hits nothing, return the background color
	if (!world.hit(r, 0.001, infinity, rec))
		return background;

	scatter_record srec;
	color emitted = rec.mat_ptr->emitted(r, rec, rec.u, rec.v, rec.p);
	if (!rec.mat_ptr->scatter(r, rec, srec))
		return emitted;

	if (srec.is_specular) {
		return srec.attenuation
			* ray_color(srec.specular_ray, background, world, lights, depth - 1);
	}

	auto light_ptr = make_shared<hittable_pdf>(lights, rec.p);
	mixture_pdf p(light_ptr, srec.pdf_ptr);

	ray scattered = ray(rec.p, p.generate(), r.time());
	auto pdf_val = p.value(scattered.direction());

	return emitted + srec.attenuation * rec.mat_ptr->scattering_pdf(r, rec, scattered)
		* ray_color(scattered, background, world, lights, depth - 1) / pdf_val;
}


hittable_list cornell_box() {
	hittable_list objects;

	auto red = make_shared<lambertian>(color(.65, .05, .05));
	auto white = make_shared<lambertian>(color(.73, .73, .73));
	auto green = make_shared<lambertian>(color(.12, .45, .15));
	auto light = make_shared<diffuse_light>(color(15, 15, 15));

	objects.add(make_shared<yz_rect>(0, 555, 0, 555, 555, green));
	objects.add(make_shared<yz_rect>(0, 555, 0, 555, 0, red));
	objects.add(make_shared<flip_face>(make_shared<xz_rect>(213, 343, 227, 332, 554, light)));
	objects.add(make_shared<xz_rect>(0, 555, 0, 555, 0, white));
	objects.add(make_shared<xz_rect>(0, 555, 0, 555, 555, white));
	objects.add(make_shared<xy_rect>(0, 555, 0, 555, 555, white));

	shared_ptr<material> aluminum = make_shared<metal>(color(0.8, 0.85, 0.88), 0.0);
	shared_ptr<hittable> box1 = make_shared<box>(point3(0, 0, 0), point3(165, 330, 165), aluminum);
	box1 = make_shared<rotate_y>(box1, 45);
	box1 = make_shared<translate>(box1, vec3(265, 0, 295));
	objects.add(box1);

	auto glass = make_shared<dielectric>(1.5);
	objects.add(make_shared<sphere>(point3(190, 90, 190), 90, glass));

	return objects;

}


int main() {

	// Parallel Setting

	omp_set_num_threads(omp_get_num_procs());
	omp_set_nested(3);

	// Image
	const auto aspect_ratio = 1.0 / 1.0;
	const int image_width = 600;
	const int image_height = static_cast<int>(image_width / aspect_ratio);
	const int samples_per_pixel = 1000;
	const int max_depth = 50;

	// World
	auto lights = make_shared<hittable_list>();
	lights->add(make_shared<xz_rect>(213, 343, 227, 332, 554, shared_ptr<material>()));
	lights->add(make_shared<sphere>(point3(190, 90, 190), 90, shared_ptr<material>()));
	//shared_ptr<hittable> lights =
		//make_shared<xz_rect>(213, 343, 227, 332, 554, shared_ptr<material>());
		//make_shared<sphere>(point3(190, 90, 190), 90, shared_ptr<material>());

	auto world = cornell_box();

	color background(0, 0, 0);

	// Camera
	point3 lookfrom(278, 278, -800);
	point3 lookat(278, 278, 0);
	vec3 vup(0, 1, 0);
	auto dist_to_focus = 10.;
	auto aperture = 0.0;
	auto vfov = 40.0;
	auto time0 = 0.0;
	auto time1 = 1.0;

	camera cam(lookfrom, lookat, vup, vfov, aspect_ratio, aperture, dist_to_focus, time0, time1);

	// Render

	std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

	//std::vector<std::vector<color>>plane_color(image_height,std::vector<color>(image_width,color(0,0,0)));

	int j = 0;

	//#pragma omp for collapse(3)
	for (j = image_height - 1; j >= 0; --j) {
		//Progress Indicator
		std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush;
		std::vector<color>line_color(image_width,color(0, 0, 0));
		int i = 0;
		
		#pragma omp parallel for //collapse(2)
		for (i = 0; i < image_width; ++i)
		{
			color pixel_color(0, 0, 0);

			//#pragma omp parallel for reduction(+:pixel_color)
			for (int s = 0; s < samples_per_pixel; ++s) {
				auto u = (i + random_double()) / (image_width - 1);
				auto v = (j + random_double()) / (image_height - 1);
				ray r = cam.get_ray(u, v);
				//#pragma omp atomic
				pixel_color += ray_color(r, background, world, lights, max_depth);
			}
			//plane_color[j][i] = pixel_color;
			line_color[i] = pixel_color;
		}
		for (i = 0; i < image_width; i++)
			write_color(std::cout, line_color[i], samples_per_pixel);
	}
	//for (j = image_height - 1; j >= 0; --j)
		//for (int i = 0; i < image_width; i++)
			//write_color(std::cout, plane_color[j][i], samples_per_pixel);

	// Progress Indicator
	std::cerr << "\nDone.\n";
}