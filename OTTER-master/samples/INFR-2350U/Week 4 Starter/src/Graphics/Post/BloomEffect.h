#pragma once

//#ifdef __BLOOMBUFFER_H__
//#define __BLOOMBUFFER_H__

#include "Graphics/Post/PostEffect.h"

class BloomEffect : public PostEffect
{

public:
	//Initializes this framebuffer
	void Init(unsigned width, unsigned height) override;

	//Applies effect to this buffer
	void ApplyEffect(PostEffect* buffer) override;

	//Reshapes the buffers
	void Reshape(unsigned width, unsigned height) override;

	//Getters
	float GetDownscale() const;
	float GetThreshold() const;
	//glm::vec2 GetPixelSize() const;
	unsigned GetPasses() const;

	//Setters
	void SetDownscale(float downscale);
	void SetThreshold(float threshold);
	void SetPasses(unsigned passes);
	//void SetPixelSize(glm::vec2 pixelSize);

private:
	float _downscale = 2.0f;
	float _threshold = 0.01f;
	unsigned _passes = 10;
	glm::vec2 m_pixelSize;


};


//#endif // !__BLOOMBUFFER_H__

