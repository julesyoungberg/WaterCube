#pragma once

#include <memory>

// #include "cinder/Cinder.h"
#include "cinder/gl/gl.h"

namespace core {

class Ssbo;
typedef std::shared_ptr<Ssbo>	SsboRef;

class Ssbo : public BufferObj {
public:
	//! Creates a shader storage buffer object with storage for \a allocationSize bytes, and filled with data \a data if it is not NULL.
	static inline SsboRef	create( GLsizeiptr allocationSize, const void *data = nullptr, GLenum usage = GL_STATIC_DRAW )
	{
		return SsboRef( new Ssbo( allocationSize, data, usage ) );
	}

	//! Bind base.
	inline void bindBase( GLuint index ) { glBindBufferBase( mTarget, index, mId );  mBase = index; }
	//! Unbinds the buffer.
	inline void unbindBase() { glBindBufferBase( mTarget, mBase, 0 ); mBase = 0; }
	//! Analogous to bufferStorage.
	inline void bufferStorage( GLsizeiptr size, const void *data, GLbitfield flags ) const { glBufferStorage( mTarget, size, data, flags ); }
protected:
	Ssbo( GLsizeiptr allocationSize, const void *data = nullptr, GLenum usage = GL_STATIC_DRAW )
		: BufferObj( GL_SHADER_STORAGE_BUFFER, allocationSize, data, usage ),
			mBase( 0 )
	{
	}
	GLuint mBase;
};

};
