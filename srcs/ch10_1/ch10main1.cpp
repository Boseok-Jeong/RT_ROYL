#define ROYL_VER 6

#include "../../lib/v2/rtweekend.h"


#include "../../lib/v2/camera.h"
#include "../../lib/v2/color.h"
#include "../../lib/v2/hittable_list.h"
#include "../../lib/v2/material.h"
#include "../../lib/v2/aarect.h"
#include "../../lib/v2/box.h"

#include "../../lib/v2/pdf.h"


#include <iostream>
color ray_color(const ray& r, const color& background, const hittable& world, int depth) {
	hit_record rec;

	// If we've exceeded the ray bounce limit, no more light is gathered
	if (depth <= 0)
		return color(0, 0, 0);

	// If the ray hits nothing, return the background color
	if (!world.hit(r, 0.001, infinity, rec))
		return background;

	ray scattered;
	color attenuation;
	color emitted = rec.mat_ptr->emitted(r, rec, rec.u, rec.v, rec.p);
	double pdf_val;
	color albedo;

	if (!rec.mat_ptr->scatter(r, rec, albedo, scattered, pdf_val))
		return emitted;

	cosine_pdf p(rec.normal);
	scattered = ray(rec.p, p.generate(), r.time());
	pdf_val = p.value(scattered.direction());

	return emitted + albedo * rec.mat_ptr->scattering_pdf(r, rec, scattered)
		* ray_color(scattered, background, world, depth - 1) / pdf_val;
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

	shared_ptr<hittable> box1 = make_shared<box>(point3(0, 0, 0), point3(165, 330, 165), white);
	box1 = make_shared<rotate_y>(box1, 15);
	box1 = make_shared<translate>(box1, vec3(265, 0, 295));
	objects.add(box1);

	shared_ptr<hittable> box2 = make_shared<box>(point3(0, 0, 0), point3(165, 165, 165), white);
	box2 = make_shared<rotate_y>(box2, -18);
	box2 = make_shared<translate>(box2, vec3(130, 0, 65));
	objects.add(box2);

	return objects;

}


int main() {

	// Image
	const auto aspect_ratio = 1.0 / 1.0;
	const int image_width = 600;
	const int image_height = static_cast<int>(image_width / aspect_ratio);
	const int samples_per_pixel = 100;
	const int max_depth = 50;

	// World
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

	for (int j = image_height - 1; j >= 0; --j) {
		//Progress Indicator
		std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush;
		for (int i = 0; i < image_width; ++i)
		{
			color pixel_color(0, 0, 0);
			for (int s = 0; s < samples_per_pixel; ++s) {
				auto u = (i + random_double()) / (image_width - 1);
				auto v = (j + random_double()) / (image_height - 1);
				ray r = cam.get_ray(u, v);
				pixel_color += ray_color(r, background, world, max_depth);
			}
			write_color(std::cout, pixel_color, samples_per_pixel);
		}
	}
	// Progress Indicator
	std::cerr << "\nDone.\n";
}