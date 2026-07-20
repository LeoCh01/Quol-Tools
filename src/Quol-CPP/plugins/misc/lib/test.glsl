#version 330 core
in vec2 v_texCoord;
out vec4 fragColor;
uniform sampler2D u_texture;
uniform float u_time;
uniform vec2  u_resolution;

const int CIRCLE = 1;
const int PLANE = 2;

struct Material {
  vec3 albedo;
  float roughness;
  bool glass;
};

struct Hittable {
  int type;
  int index;
  Material mat;
};

struct Circle {
  vec3 o;
  float r;
};

struct Plane {
  vec3 o;
  vec3 n;
};

struct Ray {
  vec3 o;
  vec3 d;
};


const vec3 lightPos = vec3(2.0, 1.0, 1.0);
const int NUM_BOUNCES = 1;

const Circle circles[] = {Circle(vec3(-1.0, 0.0, 3.0), 1.0), Circle(vec3(1.0, 0.0, 3.0), 1.0), Circle(lightPos, 0.05)};
const Plane planes[] = {Plane(vec3(0.0, -1.0, 0.0), vec3(0.0, 1.0, 0.0))};

const int NUM_HITTABLES = 3;
Hittable hittables[] = {
  Hittable(CIRCLE, 0, Material(vec3(1.0, 0.0, 0.0), 0.0, true)),
  Hittable(CIRCLE, 1, Material(vec3(0.0, 1.0, 0.0), 0.0, false)),

  // light
  // Hittable(CIRCLE, 2, Material(vec3(1.0, 0.0, 1.0), 0.0)),

  // plane
  Hittable(PLANE, 0, Material(vec3(1.0, 1.0, 1.0), 1.0, false))
};

float intersectCircle(Ray ray, Circle circle) {
  vec3 oc = ray.o - circle.o;
  float a = dot(ray.d, ray.d);
  float b = 2.0 * dot(oc, ray.d);
  float c = dot(oc, oc) - circle.r * circle.r;
  float disc = b * b - 4.0 * a * c;
  if (disc < 0.0) return -1.0;
  return (-b - sqrt(disc)) / (2.0 * a);
}

float intersectPlane(Ray ray, Plane plane) {
  float t = -dot(plane.n, ray.o - plane.o) / dot(plane.n, ray.d);
  return t > 0.0 ? t : -1.0;
}

/*
R: O + tD
C: (O - C)^2 = r^2

(O + tD - C)^2 = r^2
(V + tD)^2 = r^2
(V + tD) . (V + tD) = r^2
VV + 2tVD + t^2DD - r^2 = 0
a = DD
b = 2VD
c = VV - r^2

t = (-b ± sqrt(b^2 - 4ac)) / 2a

-VD ± sqrt(4V^2D^2 - 4DD(VV - r^2)) / DD
*/

/*
P: N . O = 0

N . (O + tD) = 0
N . O + tN . D = 0
t = -N . O / N . D

*/

float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

void main() {
  hittables[0].mat.albedo.x = sin(u_time * 10.0) * 1.0 + 1.0;
  hittables[0].mat.albedo.y = cos(u_time * 10.0) * 1.0 + 0.0;
  hittables[0].mat.albedo.z = sin(u_time * 10.0 + 0.1231) * 1.0;

  vec3 color = vec3(1);

  float aspectRatio = u_resolution.x / u_resolution.y;
  Ray ray = Ray(vec3(0, 0, 0), normalize(vec3((v_texCoord * 2.0 - 1.0) * aspectRatio * vec2(1.0, -1.0), 1.0)));

  for(int j = 0; j < NUM_BOUNCES; j++) {
    vec3 hitPoint;
    vec3 normal;

    Hittable hit;

    for(int i = 0; i < NUM_HITTABLES; i++) {
      Hittable hittable = hittables[i];
      float t = -1.0;
      
      switch(hittable.type) {
        case CIRCLE:
          t = intersectCircle(ray, circles[hittable.index]);
          break;
        case PLANE:
          t = intersectPlane(ray, planes[hittable.index]);
          break;
      }

      if(t < 0.0) {
        continue;
      }

      hit = hittable;
      hitPoint = ray.o + t * ray.d;

      switch(hittable.type) {
        case CIRCLE:
          normal = normalize(hitPoint - circles[hittable.index].o);
          break;
        case PLANE:
          normal = planes[hittable.index].n;
          break;
      }

      float cosTheta = dot(normal, normalize(lightPos - hitPoint));
      float diffuse = max(cosTheta, 0.0);

      // float specular = pow(max(dot(reflect(-normalize(lightPos - hitPoint), normal), -ray.d), 0.0), 32.0);

      // if(i == 2) {
      //   diffuse = 1.0;
      // }
      
      Ray shadowRay = Ray(hitPoint + normal * 0.01, normalize(lightPos - hitPoint));

      for(int k = 0; k < NUM_HITTABLES; k++) {
        Hittable shadowHittable = hittables[k];
        float tShadow = -1.0;

        switch(shadowHittable.type) {
          case CIRCLE:
            tShadow = intersectCircle(shadowRay, circles[shadowHittable.index]);
            break;
          case PLANE:
            tShadow = intersectPlane(shadowRay, planes[shadowHittable.index]);
            break;
        }

        if(tShadow > 0.0) {
          diffuse = 0.0;
          break;
        }
      }

      color *= hittable.mat.albedo * diffuse;
      break;
    }

    if(hit.type == 0) {
      color *= vec3(0.5, 0.5, 0.9);
      break;
    }

    ray.o = hitPoint + normal * 0.01;
    ray.d = normalize(reflect(ray.d, normal) + random(hitPoint.xy + u_time) * hit.mat.roughness);
    
  }

  fragColor = vec4(texture(u_texture, v_texCoord).rgb, color.r);
}