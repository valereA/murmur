//
//  animationWaveShader.cpp
//  murmur
//
//  Created by Julien on 16/05/13.
//
//

#include "animationWaveShader.h"
#include "soundManager.h"

//--------------------------------------------------------------
AnimationShaderWave::AnimationShaderWave(string name) : Animation(name)
{
    m_volume = 0.0f;
	m_waveIntensity = 0.5f;
	m_isBlend = true;
}

//--------------------------------------------------------------
AnimationShaderWave::~AnimationShaderWave()
{
	map<string,ShaderWave*>::iterator it = m_mapShaderWaves.begin();
	for ( ; it != m_mapShaderWaves.end() ; ++it)
		delete it->second;
}


//--------------------------------------------------------------
void AnimationShaderWave::VM_enter()
{
    M_loadShader("wave");
    //printf(">>>> %d\n", m_imgSoundInput.getPixelsRef().size());
}

//--------------------------------------------------------------
void AnimationShaderWave::VM_update(float dt)
{
    SoundManager::instance()->setVolumeSoundMainNormalized(m_volume);
}

//--------------------------------------------------------------
void AnimationShaderWave::VM_draw(float w, float h)
{
//    m_anchor.set(m_anchorNorm.x*w, m_anchorNorm.y*h);
//    printf("(%.3f,%.3f)-",m_anchor.x,m_anchor.y);
    
    ofBackground(0, 0, 0, 0);
    ofSetColor(255);
//	ofDisableAlphaBlending();
	if (m_isBlend)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		glDisable(GL_DEPTH_TEST);
	}
	
	map<string,ShaderWave*>::iterator it = m_mapShaderWaves.begin();
	for ( ; it != m_mapShaderWaves.end() ; ++it)
	{
		ShaderWave* pShaderWave = it->second;
	

	    pShaderWave->m_imgSoundInput.bind();
    	m_shader.begin();
    	m_shader.setUniform2f("resolution", 1.2f*w, 1.2f*h);
    	m_shader.setUniform2f("anchor", pShaderWave->m_anchor.x, pShaderWave->m_anchor.y);
    	m_shader.setUniform1f("texSize", pShaderWave->m_imgSoundInput.getWidth());
    	m_shader.setUniform1f("nbWaves", (float)m_mapShaderWaves.size());
    	m_shader.setUniform1f("intensityWave", m_waveIntensity);
    	m_shader.setUniformTexture("texWave", pShaderWave->m_imgSoundInput.getTextureReference(), 0);

    	glBegin(GL_QUADS);
        	glVertex3f(0,0,0);
        	glVertex3f(w,0,0);
        	glVertex3f(w,h,0);
        	glVertex3f(0,h,0);
    	glEnd();
    
    
    	m_shader.end();
    	pShaderWave->m_imgSoundInput.unbind();



	}
	if (m_isBlend)
	{
	   glDisable(GL_BLEND);
	}
}

//--------------------------------------------------------------
void AnimationShaderWave::VM_exit()
{
    
}

//--------------------------------------------------------------
void AnimationShaderWave::onNewPacket(DevicePacket* pDevicePacket, string deviceId, float x, float y)
{
    if (pDevicePacket)
	{
		accumulateVolume(pDevicePacket->m_volume, deviceId);


		map<string,ShaderWave*>::iterator it = m_mapShaderWaves.find(deviceId);
		ShaderWave* pShaderWave=0;

		if ( it == m_mapShaderWaves.end()){
			pShaderWave = new ShaderWave();
			pShaderWave->m_imgSoundInput.getPixelsRef().allocate(600, 1, 1);

	   		m_mapShaderWaves[deviceId] = pShaderWave;
		}
		else{
			pShaderWave = it->second;
		}


		if (pShaderWave)
		{
		    pShaderWave->m_anchor.set(x, y);
	        pShaderWave->m_volume = pDevicePacket->m_volume;

	        ofFloatPixels& data = pShaderWave->m_imgSoundInput.getPixelsRef();
    	    int nb = data.size();
        	for (int i=nb-1;i>=1;i--)
            	data[i] = data[i-1];
        	data[0] = pDevicePacket->m_volume;
        	pShaderWave->m_imgSoundInput.update();
		}
	}
}


//--------------------------------------------------------------
void AnimationShaderWave::createUICustom()
{
    if (mp_UIcanvas)
    {
        mp_UIcanvas->addSlider("intensity", 0.0f, 1.0f, &m_waveIntensity);
        mp_UIcanvas->addToggle("debug enable blend", &m_isBlend);
    }
}


