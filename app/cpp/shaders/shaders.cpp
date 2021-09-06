#include <string>

std::string frag_shader;
std::string vert_shader;

#define STRINGIFY(_name_) #_name_
#define SHADER(_name_)  STRINGIFY(_name_.spv)
#define SHADER_TO_STRING(shader) std::string(reinterpret_cast<const char*>(shader), static_cast<size_t>(sizeof(shader)/sizeof(shader[0])))

void LoadShaders() {
  const unsigned char frag[] = {
#include SHADER(frag)
  };
  frag_shader = SHADER_TO_STRING(frag);

  const unsigned char vert[] = {
#include SHADER(vert)
  };
  vert_shader = SHADER_TO_STRING(vert);
}
