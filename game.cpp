#include <SDL.h>
#include <string>
#include <math.h>
#include <cmath>
#include <vector>
#include <utility>
#include <optional>

const Uint8* state = SDL_GetKeyboardState(nullptr);

void set_pixel(SDL_Surface* surface, int x, int y, Uint32 pixel)
{
    Uint32* const target_pixel = (Uint32*)((Uint8*)surface->pixels
        + y * surface->pitch
        + x * surface->format->BytesPerPixel);
    *target_pixel = pixel;
}

const std::string room[] = {
    "8888888888888888888",
    "3000000000000000003",
    "3000000000000000003",
    "3000000008800000003",
    "3000000800008000003",
    "3000000000000000003",
    "3000000000000000003",
    "3000000000000000003",
    "3000100100001001003",
    "3000100100001001003",
    "3000111100001111003",
    "3000000000000000003",
    "7777777777777777777",
};


struct Player {
    float x = 4.f;
    float y = 4.f;
    float rot = 0.f;
    float speed = 0.002f;
    float rot_speed = 0.2f;
    float z = 0.f;
};

Player player = {0.f, 0.f, 0.f};

double toRadians(double degrees) {
    return degrees * M_PI / 180;
}



struct Vector2 {
    float x;
    float y;

    float magnitude() {
        return sqrt(pow(this->x, 2) + pow(this->y, 2));
    }

    Vector2 normalize() {
        float mag = this->magnitude();
        return { this->x / mag, this->y / mag };
    }

    Vector2 operator-(Vector2 vec) {
        return { this->x - vec.x, this->y - vec.y };
    }

    Vector2 abs() {
        return { std::abs(this->x), std::abs(this->y) };
    }

    float cross(Vector2 v2) {
        return v2.x * this->y - this->x * v2.y;
    }

    float dot(Vector2 v2) {
        return this->x * v2.x + this->y * v2.y;
    }
};

const std::vector<std::pair<Vector2, Vector2>> roomLines = std::vector<std::pair<Vector2, Vector2>>({
    std::pair<Vector2, Vector2>({-5,5}, {5,5}),
    std::pair<Vector2, Vector2>({5,5}, {5,-5}),
    std::pair<Vector2, Vector2>({5,-5}, {-5,-5}),
    std::pair<Vector2, Vector2>({-5,-5}, {-5,5}),
});


class Ray {


public:
    void shoot();
    Vector2 calcHit();
    Ray(float origin_x, float origin_y, float rot) {
        this->origin_x = origin_x;
        this->origin_y = origin_y;
        this->rot = rot;
    }
    float origin_x;
    float origin_y;
    float rot;
    float dist = -1;
    char hitType;
    char maxDist = 1000;
    float getRadians() {
        return toRadians(rot);
    }
    
};

Vector2 Ray::calcHit() {
    float radians = toRadians(this->rot);
    float distance = maxDist;
    if (this->dist != -1) {
        distance = this->dist;
    }
    return { std::sin(radians) * distance, std::cos(radians) * distance };
}

std::optional<Vector2> intersectionPoint(Ray ray, Vector2 p3, Vector2 p4) {
    //Formula courtesy of: https://rootllama.wordpress.com/2014/06/20/ray-line-segment-intersection-test-in-2d/
    Vector2 o = { ray.origin_x, ray.origin_y };

    Vector2 v1 = o - p3;
    Vector2 v2 = p4 - p3;
    Vector2 v3 = { -std::sin(ray.getRadians()), std::cos(ray.getRadians()) }; // inverse of direction
    // (v2 * v1)
    float t1_top = v1.cross(v2);
    float t1_bottom = v2.dot(v3);

    float t1 = t1_top / t1_bottom;

    float t2 = v1.dot(v3) / v2.dot(v3);

    if (t1 >= 0 && t2 <= 1) {
        Vector2 intersect = { t1, t2 };
        return std::make_optional<Vector2>(intersect);
    }
    return std::nullopt;
}

float resolution = 0.02;
void Ray::shoot() {
    bool hit = false;

    Vector2 pos = { this->origin_x, this->origin_y };

    int iter = 0;

    float radians = toRadians(this->rot);

    Vector2* closestPoint = nullptr;
    float closestDistance = INFINITY;

    for (std::pair<Vector2, Vector2> line : roomLines) {
        auto point = intersectionPoint(*this, line.first, line.second);
        if (point.has_value()) {
            auto certainPoint = point.value();

            float distance = sqrt(pow(this->origin_x - certainPoint.x, 2) + pow(this->origin_y - certainPoint.y, 2));

            if (closestPoint == nullptr || closestDistance > distance) {
                closestPoint = &certainPoint;
                closestDistance = distance;
                continue;
            }
        }
    }

    if (closestPoint == nullptr) {
        printf("Hole Detected in map.");
    }
    else {
        this->hitType = '1';
    }



    // Pythagorean Theorem to get the distance between 2 points
    // sqrt( (x1 - x2)² + (y1 - y2)² )
    // in the above case, point 1 is the origin, and point 2 is the position of the ray hit
    this->dist = closestDistance;
}




Vector2 getSurfaceNormalFromRay(Ray ray) {
    float radians = toRadians(ray.rot);
    float point_pos_x = ray.origin_x + sin(radians) * ray.dist;
    float point_pos_y = ray.origin_y + cos(radians) * ray.dist;

    Vector2 wpv = {point_pos_x - player.x, point_pos_y - player.y};
    wpv = wpv.normalize();
    Vector2 camera_direction = { sin(toRadians(player.rot)), cos(toRadians(player.rot)) };
    camera_direction = camera_direction.normalize();
    float dot = wpv.x * camera_direction.x + wpv.y * camera_direction.y;
    if (dot == 0) {
        return {0, 0};
    }
    
    return {wpv.x - (dot * camera_direction.x), wpv.y - (dot * camera_direction.y)};
}

Uint32 getColorFromChar(char c) {
    switch (c) {
    case '0': return 0x0;
    case '1': return 0xffa000;
    case '2': return 0x00ff00;
    case '3': return 0xff0000;
    case '4': return 0xffff00;
    case '5': return 0xff0000;
    case '6': return 0xff0000;
    case '7': return 0x0000ff;
    case '8': return 0x00ff00;
    case '9': return 0x0000ff;
    default: return 0x0;
    }  
}

float fov = 15;
float wall_scale_factor = 23;
float distance_scale_factor = 18;
float x_offset = 0.5;

void runGameLoop(SDL_Surface* surface, double frameDelta) {
    fov += ((state[SDL_SCANCODE_O] ? 1 : 0) - (state[SDL_SCANCODE_L] ? 1 : 0)) * frameDelta * 0.01;
    float center_y = surface->h / 2;
    float center_x = surface->w / 2;

    // wall_scale_factor += ((state[SDL_SCANCODE_I] ? 1 : 0) - (state[SDL_SCANCODE_K] ? 1 : 0)) * frameDelta * 0.01;
    // distance_scale_factor += ((state[SDL_SCANCODE_U] ? 1 : 0) - (state[SDL_SCANCODE_J] ? 1 : 0)) * frameDelta * 0.01;
    // x_offset += ((state[SDL_SCANCODE_Y] ? 1 : 0) - (state[SDL_SCANCODE_H] ? 1 : 0)) * frameDelta * 0.01;

    if(state[SDL_SCANCODE_P]) {
        printf("fov: %f\nwsf: %f\ndsf: %f\n", fov, wall_scale_factor, distance_scale_factor);
    }
    // Causes crashes lol
    // resolution += ((state[SDL_SCANCODE_Y] ? 1 : 0) - (state[SDL_SCANCODE_H] ? 1 : 0)) * frameDelta * 0.001;

    float radians = toRadians(player.rot);
    float move_x = (state[SDL_SCANCODE_W] ? 1 : 0) - (state[SDL_SCANCODE_S] ? 1 : 0);
    float move_y = (state[SDL_SCANCODE_D] ? 1 : 0) - (state[SDL_SCANCODE_A] ? 1 : 0);

    float move_r = (state[SDL_SCANCODE_RIGHT] ? 1 : 0) - (state[SDL_SCANCODE_LEFT] ? 1 : 0);

    player.rot += move_r * player.rot_speed * frameDelta;


    player.x += std::cos(radians) * player.speed * move_x * frameDelta;
    player.y += std::sin(radians) * player.speed * move_x * frameDelta;

    player.x += std::cos(radians + 90) * player.speed * move_y * frameDelta;
    player.y += std::sin(radians + 90) * player.speed * move_y * frameDelta;

    if (player.y > room->length()) {
        player.y = 2;
    }
    if (player.x > room[0].length()) {
        player.x = 2;
    }


    for (int x = 0; x < surface->w; x++) {
        Vector2 radian_pl = {toRadians(player.rot - 90), toRadians(player.rot)};

        Vector2 sideways = {cos(radian_pl.x), sin(radian_pl.x)};
        Vector2 forward = {cos(radian_pl.y), sin(radian_pl.y)};

        Ray ray = Ray(player.x + (sideways.x * x_offset), player.y + (sideways.x * x_offset), player.rot + ((x - center_x) / fov));
        ray.shoot();
        for (int y = 0; y < surface->h; y++) {
            if (abs(center_y - y) < (distance_scale_factor / ray.dist) * wall_scale_factor) {
                auto color = getColorFromChar(ray.hitType);
                int r = (color >> 24) & 0xFF;
                int g = (color >> 16) & 0xFF;
                int b = (color >> 8) & 0xFF;

                float calcdist = (10 - ray.dist);

                if(calcdist < 0) {
                    calcdist = 0;
                }

                r *= calcdist / 10;
                g *= calcdist / 10;
                b *= calcdist / 10;

                if(r > 255) {
                    r = 255;
                }
                if(g > 255) {
                    g = 255;
                }
                if(b > 255) {
                    b = 255;
                }


                color = (r << 24) | (g << 16) | (b << 8) | (color & 0xFF);

                set_pixel(surface, x, y, color);
            }
            else {
                set_pixel(surface, x, y, y > center_y ? 0xaaaaaa : 0xffffff);
            }
        }

    }
    
}

