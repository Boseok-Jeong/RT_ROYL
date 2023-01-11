#include "../../lib/v3/rtweekend.h"

#include "../../lib/v3/aarect.h"
#include "../../lib/v3/box.h"
#include "../../lib/v3/camera.h"
#include "../../lib/v3/color.h"
#include "../../lib/v3/hittable_list.h"
#include "../../lib/v3/material.h"
#include "../../lib/v3/sphere.h"

#include <omp.h>

using namespace std;

using std::vector;

#include <iostream>
color ray_color(
	const ray& r,
	const color& background,
	const hittable& world,
	shared_ptr<hittable> pdf_area,
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
			* ray_color(srec.scatter_ray, background, world, pdf_area, depth - 1);
	}

	auto pdf_area_ptr = make_shared<hittable_pdf>(pdf_area, rec.p);
	mixture_pdf p(pdf_area_ptr, srec.pdf_ptr);

	ray scattered = ray(rec.p, p.generate(), r.time());
	auto pdf_val = p.value(scattered.direction());

	return emitted + srec.attenuation * rec.mat_ptr->scattering_pdf(r, rec, scattered)
		* ray_color(scattered, background, world, pdf_area, depth - 1) / pdf_val;
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
	omp_set_nested(1);

	// Image
	const auto aspect_ratio = 1.0 / 1.0;
	const int image_width = 200;
	const int image_height = static_cast<int>(image_width / aspect_ratio);
	const int samples_per_pixel = 100;
	const int max_depth = 50;

	// World
	auto pdf_area = make_shared<hittable_list>();
	pdf_area->add(make_shared<xz_rect>(213, 343, 227, 332, 554, shared_ptr<material>()));
	pdf_area->add(make_shared<sphere>(point3(190, 90, 190), 90, shared_ptr<material>()));
	//shared_ptr<hittable> pdf_area =
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

	cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

	vector<vector<vector<color>>>str_color(image_height,vector<vector<color>>(image_width, vector<color>(samples_per_pixel,color(0,0,0))));

	cerr << "\rLoop Start " << flush;

	int j(0), i(0), s(0);// , cnt(0);

	#pragma omp parallel for private(i, s)//collapse(3)
	for (j = image_height - 1; j >= 0; --j) {
		
		//#pragma omp atomic
		//cnt++;

		//Progress Indicator
		//std::cerr << "\rScanlines remaining: " << cnt << ' ' << std::flush;
		
		for (i = 0; i < image_width; ++i)
		{
			//color pixel_color(0, 0, 0);

			for (s = 0; s < samples_per_pixel; ++s) {
				auto u = (i + random_double()) / (image_width - 1);
				auto v = (j + random_double()) / (image_height - 1);
				ray r = cam.get_ray(u, v);
				
				//pixel_color += ray_color(r, background, world, pdf_area, max_depth);
				str_color[j][i][s] = ray_color(r, background, world, pdf_area, max_depth);
			}
			//write_color(std::cout, pixel_color, samples_per_pixel);
		}

	}
	vector<vector<color>> pixel_img(image_height, vector<color>(image_width, color(0, 0, 0)));

	for (j = image_height - 1; j >= 0; --j) {
		//Progress Indicator
		cerr << "\rScanlines remaining: " << j << ' ' << flush;

		for (i = 0; i < image_width; ++i)
		{
			//color pixel_color(0, 0, 0);

			for (s = 0; s < samples_per_pixel; ++s)
				pixel_img[j][i] += str_color[j][i][s];
			
			write_color(cout, pixel_img[j][i], samples_per_pixel);
		}

	}
	
	// Progress Indicator
	std::cerr << "\nDone.\n";
}