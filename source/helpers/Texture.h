#include <GL/glew.h>
#include "Bitmap.h"

namespace helpers {
    class Texture {
    public:
        Texture(const Bitmap& bitmap,
                GLint minMagFiler = GL_LINEAR,
                GLint wrapMode = GL_CLAMP_TO_EDGE);
        
        ~Texture();
		GLuint object() const;
		GLfloat originalWidth() const;
		GLfloat originalHeight() const;
        
    private:
        GLuint _object;        
        Texture(const Texture&);
        const Texture& operator=(const Texture&);
    };
    
}
