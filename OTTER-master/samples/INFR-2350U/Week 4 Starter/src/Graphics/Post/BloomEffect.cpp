#include "BloomEffect.h"
//ALL This Code is referenced from Spritelib

void BloomEffect::Init(unsigned width, unsigned height)
{
    int index = int(_buffers.size());
    _buffers.push_back(new Framebuffer());
	_buffers[index]->AddColorTarget(GL_RGBA8);
	_buffers[index]->Init(width, height);
	index++;
	_buffers.push_back(new Framebuffer());
	_buffers[index]->AddColorTarget(GL_RGBA8);
	_buffers[index]->Init(unsigned(width / _downscale), unsigned(height / _downscale));
	index++;
	_buffers.push_back(new Framebuffer());
	_buffers[index]->AddColorTarget(GL_RGBA8);
	_buffers[index]->Init(unsigned(width / _downscale), unsigned(height / _downscale));
	index++;
	_buffers.push_back(new Framebuffer());
	_buffers[index]->AddColorTarget(GL_RGBA8);
	_buffers[index]->Init(width, height);

	//Check if the shader is initialized
	//Load in the shader
	int index2 = int(_shaders.size());
	_shaders.push_back(Shader::Create());
	_shaders[index2]->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
	_shaders[index2]->LoadShaderPartFromFile("shaders/Bloom/BloomPassthrough.frag.glsl", GL_FRAGMENT_SHADER);
	_shaders[index2]->Link();

	index2++;
	_shaders.push_back(Shader::Create());
	_shaders[index2]->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
	_shaders[index2]->LoadShaderPartFromFile("shaders/Bloom/BloomHighPass.frag.glsl", GL_FRAGMENT_SHADER);
	_shaders[index2]->Link();

	index2++;
	_shaders.push_back(Shader::Create());
	_shaders[index2]->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
	_shaders[index2]->LoadShaderPartFromFile("shaders/Bloom/BlurHorizontal.frag.glsl", GL_FRAGMENT_SHADER);
	_shaders[index2]->Link();

	index2++;
	_shaders.push_back(Shader::Create());
	_shaders[index2]->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
	_shaders[index2]->LoadShaderPartFromFile("shaders/Bloom/BlurVertical.frag.glsl", GL_FRAGMENT_SHADER);
	_shaders[index2]->Link();

	index2++;
	_shaders.push_back(Shader::Create());
	_shaders[index2]->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
	_shaders[index2]->LoadShaderPartFromFile("shaders/Bloom/BloomComposite.frag.glsl", GL_FRAGMENT_SHADER);
	_shaders[index2]->Link();

	//Pixel size
	m_pixelSize = glm::vec2(1.f / width, 1.f / height);

	


}

void BloomEffect::ApplyEffect(PostEffect* buffer)
{
	//Draws previous buffer to first render target
	BindShader(0);

	buffer->BindColorAsTexture(0, 0, 0);

	_buffers[0]->RenderToFSQ();

	buffer->UnbindTexture(0);

	UnbindShader();

	//Performs high pass on the first render target
	BindShader(1);
	_shaders[1]->SetUniform("u_Threshold", _threshold);

	BindColorAsTexture(0, 0, 0);

	_buffers[1]->RenderToFSQ();

	UnbindTexture(0);

	UnbindShader();

	//Computes blur, vertical and horizontal
	for (unsigned i = 0; i < _passes; i++)
	{
		//Horizontal pass
		BindShader(2);
		_shaders[2]->SetUniform("u_PixelSize", m_pixelSize.x);

		BindColorAsTexture(1, 0, 0);

		_buffers[2]->RenderToFSQ();

		UnbindTexture(0);

		UnbindShader();

		//Vertical pass
		BindShader(3);
		_shaders[3]->SetUniform("u_PixelSize", m_pixelSize.y);

		BindColorAsTexture(2, 0, 0);

		_buffers[1]->RenderToFSQ();

		UnbindTexture(0);

		UnbindShader();
	}

	//Composite the scene and the bloom
	BindShader(4);

	buffer->BindColorAsTexture(0, 0, 0);
	BindColorAsTexture(1, 0, 1);

	_buffers[0]->RenderToFSQ();

	UnbindTexture(1);
	UnbindTexture(0);

	UnbindShader();
}

void BloomEffect::Reshape(unsigned width, unsigned height)
{
	_buffers[0]->Reshape(width, height);
	_buffers[1]->Reshape(unsigned(width / _downscale), unsigned(height / _downscale));
	_buffers[2]->Reshape(unsigned(width / _downscale), unsigned(height / _downscale));
	_buffers[3]->Reshape(width, height);
}

float BloomEffect::GetDownscale() const
{
    return _downscale;
}

float BloomEffect::GetThreshold() const
{
    return _threshold;
}

//glm::vec2 BloomEffect::GetPixelSize() const
//{
//	return m_pixelSize;
//}

unsigned BloomEffect::GetPasses() const
{
	return _passes;
}

void BloomEffect::SetDownscale(float downscale)
{
	_downscale = downscale;
	Reshape(_buffers[0]->_width, _buffers[0]->_height);

}

void BloomEffect::SetThreshold(float threshold)
{
	_threshold = threshold;
}

void BloomEffect::SetPasses(unsigned passes)
{
	_passes = passes;
}

//void BloomEffect::SetPixelSize(glm::vec2 pixelSize)
//{
//	m_pixelSize = pixelSize;
//}
