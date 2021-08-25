#include "engine.h"
#include <limits.h>
#include <math.h>
#define __USE_GNU
#include <SDL2/SDL.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "camera.h"
#include "config.h"
#include "framebuffer_wrapper.h"
#include "material/dielectric.h"
#include "material/lambertian.h"
#include "material/metal.h"
#include "ray.h"
#include "shapes.h"
#include "utils.h"
#include "vec3.h"

struct render_thread_args
{
    Color *framebuffer;
    size_t width;
    size_t height;
    size_t s_x;
    size_t s_y;
};

struct render_part_args
{
    Color *framebuffer;
    size_t width;
    size_t height;
    double x_from;
    double y_from;
    double x_max;
    double y_max;
    size_t sample_count;
};

static const int sample_count_x = RENDER_THREAD;
static const int sample_step_x = SCRN_WIDTH / sample_count_x;
static const int sample_count_y = RENDER_THREAD;
static const int sample_step_y = SCRN_HEIGHT / sample_count_y;

static bool stop;
static Camera camera;

struct render_thread_args args[MAX_RENDER_THREAD];
pthread_t thr[MAX_RENDER_THREAD] = {0};

static Vec3 calculate_void_color(Ray from)
{
    double t;
    Vec3 unit_direction;

    unit_direction = vec3_unit(from.direction);
    t = 0.5 * (unit_direction.y + 1);

    return vec3_add(vec3_mul_val(vec3_create(1, 1, 1), 1 - t), vec3_mul_val(vec3_create(0.5, 0.7, 1), t));
}

static Vec3 calculate_ray_color(Ray from, int depth)
{
    HitRecord record = {0};

    if (depth > MAX_BOUNCE_DEPTH)
    {
        return vec3_create(0, 0, 0);
    }

    if (hit_call_all_object(from, 0.001, __builtin_inff64(), &record))
    {
        Ray forked_ray = from;
        Vec3 attenuation = vec3_create(1, 1, 1);

        if (record.material.material_callback(&from, &record, &attenuation, &forked_ray, record.material.data))
        {
            Vec3 next = calculate_ray_color(forked_ray, depth + 1);
            return vec3_mul(attenuation, next);
        }
        else
        {
            return vec3_create(0, 0, 0);
        }
    }
    else
    {
        return calculate_void_color(from);
    }
}

static void render_update_part(struct render_part_args arg)
{

    /*ù  uint32_t t = SDL_GetTicks();*/

    Vec3 current_color;
    Color final_color;

    Color previous_color;
    Ray r;
    size_t x, y;
    double u, v;

    for (x = arg.x_from; x < arg.x_max; x += 1)
    {
        for (y = arg.y_from; y < arg.y_max; y += 1)
        {
            u = ((double)x + random_double()) / (double)(arg.width - 1);
            v = ((double)y + random_double()) / (double)(arg.height - 1);

            r = get_camera_ray(&camera, u, v);

            current_color = (calculate_ray_color(r, 0));

            /* add current sample to sum */
            if (arg.sample_count != 0)
            {
                previous_color = get_pixel(arg.framebuffer, x, y, arg.width);

                current_color = vec3_div_val(
                    vec3_add(
                        vec3_mul_val(vec_from_color(previous_color), arg.sample_count - 1), current_color),
                    arg.sample_count);
            }

            final_color = vec_to_color(current_color);

            set_pixel(arg.framebuffer, x, y, arg.width, final_color);

            if (stop)
            {
                return;
            }
        }
    }

}
static void *render_update_part_thread(void *arg)
{
    struct render_thread_args *args = arg;
    struct render_part_args render_argument;

    double u = (double)args->s_x;
    double v = (double)args->s_y;

    render_argument.sample_count = 1;
    render_argument.width = args->width;
    render_argument.height = args->height;
    render_argument.framebuffer = args->framebuffer;
    render_argument.x_from = u;
    render_argument.y_from = v;
    render_argument.x_max = sample_step_x + u;
    render_argument.y_max = sample_step_y + v;

    while (render_argument.sample_count < 128)
    {
        render_update_part(render_argument);
        render_argument.sample_count++;
    }

    return NULL;
}

void render_update(Color *framebuffer, size_t width, size_t height)
{
    size_t i = 0;
    size_t s_x = 0;
    size_t s_y = 0;

    while (true)
    {
        i++;
        if (thr[i] == 0)
        {
            args[i].framebuffer = framebuffer;
            args[i].width = width;
            args[i].height = height;
            args[i].s_x = s_x;
            args[i].s_y = s_y;

            pthread_create(&thr[i], NULL, render_update_part_thread, &args[i]);
        }

        i++;
        s_x += sample_step_x;
        if (s_x >= SCRN_WIDTH)
        {
            s_x = 0;
            s_y += sample_step_y;
            if (s_y >= SCRN_HEIGHT)
            {
                break;
            }
        }
    }
}

pthread_mutex_t main_mutex;

static void random_scene(void)
{

    int a, b;

    add_hitable_object((HitCallback)hit_sphere_object_callback, sphere_create(1000, vec3_create(0, -1000, -1)), lambertian_create(vec3_create(0.5, 0.5, 0.5)));

    for (a = -11; a < 11; a++)
    {
        for (b = -11; b < 11; b++)
        {
            Material result_material;

            double material = random_double();
            Vec3 center = vec3_create(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double());

            if (vec3_length(vec3_sub(center, vec3_create(4, 0.2, 0))) > 0.9)
            {
                if (material < 0.8)
                {
                    Vec3 random_albedo = vec3_create(random_double(), random_double(), random_double());
                    result_material = lambertian_create(vec3_mul(random_albedo, random_albedo));
                }
                else if (material < 0.95)
                {
                    Vec3 random_albedo = vec3_create(random_double(), random_double(), random_double());
                    double fuzz = random_double() / 2;
                    result_material = metal_create(random_albedo, fuzz);
                }
                else
                {
                    result_material = dieletric_create(1.5);
                }

                add_hitable_object((HitCallback)hit_sphere_object_callback, sphere_create(0.2, center), result_material);
            }
        }
    }

    add_hitable_object((HitCallback)hit_sphere_object_callback, sphere_create(1.0, vec3_create(0, 1, 0)), dieletric_create(1.5));
    add_hitable_object((HitCallback)hit_sphere_object_callback, sphere_create(1.0, vec3_create(-4, 1, 0)), lambertian_create(vec3_create(0.4, 0.2, 0.1)));
    add_hitable_object((HitCallback)hit_sphere_object_callback, sphere_create(1.0, vec3_create(4, 1, 0)), metal_create(vec3_create(0.7, 0.6, 0.5), 0));
}
void render_init(void)
{
    struct camera_config camera_config;

    stop = false;
    camera_config.position = vec3_create(13, 2, 3);
    camera_config.lookat = vec3_create(0, 0, 0);
    camera_config.up = vec3_create(0, 1, 0);
    camera_config.aspect = ((float)SCRN_WIDTH / (float)SCRN_HEIGHT);
    camera_config.vfov = 20;
    camera_config.aperture = 0.1;
    camera_config.focus_distance = 10;

    camera = create_camera(camera_config);

    random_scene();

    pthread_mutex_init(&main_mutex, NULL);
}

void render_deinit(void)
{
    size_t i = 0;

    stop = true;

    for (i = 0; i < MAX_RENDER_THREAD; i++)
    {
        if (thr[i] != 0)
            pthread_join(thr[i], NULL);
    }

    hit_destroy_all_objects();
    pthread_mutex_destroy(&main_mutex);
}

void framebuffer_lock(void)
{
    pthread_mutex_lock(&main_mutex);
}

void framebuffer_unlock(void)
{
    pthread_mutex_unlock(&main_mutex);
}
