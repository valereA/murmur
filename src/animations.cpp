/*
 *  animations.cpp
 *  versatubes
 *
 *  Created by 2Roqs on 30/04/12.
 *  Copyright 2012 2Roqs. All rights reserved.
 *
 */

#include "animations.h"
#include "globals.h"
#include "utils.h"
#include "ofxJSScript.h"
#include "soundManager.h"


#define ANIM_UI_WIDTH_DEFAULT   330
#define ANIM_UI_HEIGHT_DEFAULT   16




#define ____________AnimationSoundPlayer____________
AnimationSoundPlayer::AnimationSoundPlayer()
{
	m_volume = 1.0f;
}

//--------------------------------------------------------------
void AnimationSoundPlayer::add(string name)
{
    m_listSoundNames.push_back( name );
}

//--------------------------------------------------------------
void AnimationSoundPlayer::playRandom(vector<int>& listSpeakers)
{
	// TODO : cache this!!!
	int nbSpeakers = listSpeakers.size();
	int speakers[nbSpeakers];
	for (int i=0;i<nbSpeakers;i++){
		speakers[i] = listSpeakers.at(i);
	}
	
	playRandom(speakers, nbSpeakers);
}

//--------------------------------------------------------------
void AnimationSoundPlayer::playRandom(int* speakers, int nbSpeakers)
{
//	ofLog() << m_listSoundNames.size();
    if (m_listSoundNames.size()>=2)
    {
        int rndIndex = -1;
        do
        {
            rndIndex = floor(ofRandom(1.0) * m_listSoundNames.size());
            if (rndIndex == m_listSoundNames.size())
                rndIndex = 0;
        }
        while (rndIndex==m_lastPlayedIndex);
        m_lastPlayedIndex = rndIndex;

        SoundManager::instance()->playSound( m_listSoundNames[rndIndex],false,m_volume,speakers,nbSpeakers );
    }
    else
    if (m_listSoundNames.size()==1)
        SoundManager::instance()->playSound( m_listSoundNames[0],false,m_volume,speakers,nbSpeakers );

}

//--------------------------------------------------------------
void AnimationSoundPlayer::print(string title)
{
	ofLog() << "+AnimationSoundPlayer for " << title;
	for (int i=0; i<m_listSoundNames.size(); i++){
		ofLog() << " - " << m_listSoundNames[i];
	}
}



#define ____________Animation____________
std::map<JSObject*, Animation*> Animation::sm_mapJSObj_Anim;

//--------------------------------------------------------------
Animation::Animation()
{
	M_zeroAll();
}

//--------------------------------------------------------------
Animation::Animation(string name_)
{
	M_zeroAll();
	m_name			= name_;
}

//--------------------------------------------------------------
Animation::Animation(string name_,string pathAbsScript_)
{
	M_zeroAll();
	m_name			= name_;
	m_pathAbsScript = pathAbsScript_;

	mp_obj = JS_NewObject(ofxJSGetContext(), NULL, NULL, ofxJSGetGlobalObj());
    JSBool ok = JS_SetPrivate(ofxJSGetContext(), mp_obj, (void*)this);
	JS_AddNamedRoot(ofxJSGetContext(), &mp_obj, m_name.c_str());
    
    
    ofxJSDefineFunctionObject(mp_obj, "newSlider", Animation::jsNewSlider, 4);
    ofxJSDefineFunctionObject(mp_obj, "newToggle", Animation::jsNewToggle, 4);


    sm_mapJSObj_Anim[mp_obj] = this;

    // printf("#### setting private %s for obj %p, this=%p\n", ok == JS_TRUE ? "OK" : "NO", mp_obj, this);
}

//--------------------------------------------------------------
Animation::~Animation(){
//	printf("~Animation()\n");
	delete mp_script;
    delete mp_UIcanvas;
}

//--------------------------------------------------------------
void Animation::M_zeroAll()
{
	mp_script		= 0;
	mp_obj			= 0;
	m_isAutoClear	= true;
    mp_UIcanvas     = 0;
}

//--------------------------------------------------------------
bool Animation::M_loadScript(const char* s)
{
//	printf("M_loadScript() > path='%s'\n", s);
	string pathAbs = s;
	if (pathAbs != "")
	{
		mp_script = ofxJSLoad(pathAbs,"___tmpScript___");
		if (mp_script){

//			if (ofxJSEval(mp_script))
			if (ofxJSEvalOnObject(mp_script, mp_obj))
			{
//				printf("OK > evaluated '%s'\n", s);
				//ofxJSValue retVal;
				return true;
			}
			else{
				printf("ERROR > cannot eval script '%s'\n", s);
				M_deleteScript();
			}
		}
		else{
			printf("ERROR > cannot load script '%s'\n", s);
			M_deleteScript();
		}
	}
	else
		printf("ERROR > path for script is empty...\n");

	return false;
}

//--------------------------------------------------------------
bool Animation::M_reloadScript()
{
	return M_loadScript(m_pathAbsScript.c_str());
}

//--------------------------------------------------------------
void Animation::VM_setArgs(string args)
{
}

//--------------------------------------------------------------
void Animation::createUI()
{
    if (mp_UIcanvas)
    {
        mp_UIcanvas->addWidgetDown(new ofxUILabel(m_name, OFX_UI_FONT_LARGE));
        mp_UIcanvas->addWidgetDown(new ofxUISpacer(ANIM_UI_WIDTH_DEFAULT, 2));
    }
    
    if (mp_UIcanvas && mp_obj)
    {
		
        ofxJSValue retVal;
        ofxJSCallFunctionNameObject_NoArgs_IfExists(mp_obj,"createUI",retVal);
    }
    
    createUICustom();
	createUISound();

    ofAddListener(mp_UIcanvas->newGUIEvent, this, &Animation::guiEvent);
}


//--------------------------------------------------------------
void Animation::registerSoundTags(vector<string>& soundTags)
{
	soundTags.push_back("hit");
}

//--------------------------------------------------------------
void Animation::playSound(string deviceId)
{
	Device* pDevice = getDevice(deviceId);
	if (pDevice)
		m_soundPlayer.playRandom(pDevice->m_listSpeakerIds);
}

//--------------------------------------------------------------
void Animation::createUISound()
{
	if (mp_UIcanvas == 0) return;
	
	mp_UIcanvas->addWidgetDown(new ofxUILabel("Sounds", OFX_UI_FONT_MEDIUM));
    mp_UIcanvas->addWidgetDown(new ofxUISpacer(ANIM_UI_WIDTH_DEFAULT, 2));

	// Soundplayer properties
	mp_UIcanvas->addWidgetDown(new ofxUISlider("volume", 0.0f, 1.0f, 1.0f,310,ANIM_UI_HEIGHT_DEFAULT));
	
	// Sound lists (filtered by tags)
	vector<string> soundTags;
	soundTags.push_back(m_name);
	registerSoundTags(soundTags);
	
	string tags="";
	string tags_sep="";
	for (int i=0;i<soundTags.size();i++){
		tags += tags_sep+"'"+soundTags[i]+"'";
		tags_sep=", ";
	}
	
	
	m_listSoundNames = SoundManager::instance()->getListSoundsNameWithTag(soundTags);
	int nbSounds = m_listSoundNames.size();
	for (int i=0; i<nbSounds; i++)
	{
		ofxUIToggle* pSoundToggle = new ofxUIToggle(m_listSoundNames[i],false,20,20);
		if (i%2 == 0)
			mp_UIcanvas->addWidgetDown(pSoundToggle);
		else
			mp_UIcanvas->addWidgetRight(pSoundToggle);
	}

	mp_UIcanvas->addWidgetDown(new ofxUITextArea("tags","Sound tags are "+tags+".",320,30,0,0,OFX_UI_FONT_SMALL));
}

//--------------------------------------------------------------
ofxUICanvas* Animation::getUI()
{
	return mp_UIcanvas;
}

//--------------------------------------------------------------
ofxUICanvas* Animation::showUI(bool is)
{
    if (mp_UIcanvas){
        mp_UIcanvas->setVisible(is);
//        printf(">>> Animation::showUI() for %s", m_name.c_str());
    }
    return mp_UIcanvas;
}

//--------------------------------------------------------------
ofxUICanvas* Animation::hideUI()
{
    return showUI(false);
}

//--------------------------------------------------------------
bool Animation::guiEventTogglesSound(string name)
{
	bool isSoundToggle = false;
	vector<string> listSoundNames = SoundManager::instance()->getListSoundsName();
	for (int i=0;i<listSoundNames.size();i++){
		if (name == listSoundNames[i]){
			isSoundToggle = true;
			break;
		}
	}
	return isSoundToggle;
}


//--------------------------------------------------------------
void Animation::jsCallSoundChanged()
{
  if (mp_obj)
  {
	  ofxJSValue retVal;
	  ofxJSCallFunctionNameObject_NoArgs_IfExists(mp_obj, "soundsChanged", retVal);
  }
}

//--------------------------------------------------------------
Device* Animation::getDevice(string deviceId)
{
    DeviceManager* pDeviceManager = GLOBALS->mp_deviceManager;
	if (pDeviceManager)
	{
		return pDeviceManager->getDeviceById(deviceId);
	}
	return 0;
}

//--------------------------------------------------------------
Device* Animation::getDeviceCurrent()
{
    DeviceManager* pDeviceManager = GLOBALS->mp_deviceManager;
	if (pDeviceManager)
	{
		return pDeviceManager->getDeviceCurrent();
	}
	return 0;
}


//--------------------------------------------------------------
bool Animation::updateDevicePosition(string deviceId, float x, float y)
{
	bool is = false;
	if (m_devicePositions.find(deviceId) == m_devicePositions.end())
	{
		m_devicePositions[deviceId] = ofVec2f(x,y);
		is = true;
	}
	else
	{
		ofVec2f posNow = m_devicePositions[deviceId];
		if (posNow.x != x || posNow.y != y){
			is = true;
			m_devicePositions[deviceId].set(x,y);
		}
	}

	return is;
}


//--------------------------------------------------------------
void Animation::guiEvent(ofxUIEventArgs &e)
{
	string name = e.widget->getName();
	int kind = e.widget->getKind();


    if (kind == OFX_UI_WIDGET_SLIDER_H || kind == OFX_UI_WIDGET_SLIDER_V)
    {
		if (name == "volume")
		{
			m_soundPlayer.setVolume( ((ofxUISlider*) e.widget)->getScaledValue() );
		}
		else
		{
			if (mp_obj)
    	    {
			    ofxJSValue retVal;
        	    ofxJSValue args[2];
            	args[0] = string_TO_ofxJSValue( name );
            	args[1] = float_TO_ofxJSValue( ((ofxUISlider*) e.widget)->getScaledValue() );
				ofxJSCallFunctionNameObject_IfExists(mp_obj,"eventUI", args,2,retVal);
			}
		}
    }
	else
	if (kind == OFX_UI_WIDGET_TOGGLE)
	{
		// TODO : not cool here

		if (guiEventTogglesSound(name))
		{
			m_soundPlayer.m_listSoundNames.clear();
			for (int i=0; i<m_listSoundNames.size(); i++){
				ofxUIToggle* pSoundToggle = (ofxUIToggle*) mp_UIcanvas->getWidget( m_listSoundNames[i] );
				if (pSoundToggle->getValue()){
					// ofLog() << m_name << " / " << listSoundNames[i];
					m_soundPlayer.add( m_listSoundNames[i] );
				}
			}


			// jsCallSoundChanged();
		}
		else
		{
			if (mp_obj)
    	    {
				bool valToggle = ((ofxUIToggle*) e.widget)->getValue() ;

			    ofxJSValue retVal;
	            ofxJSValue args[2];
    	        args[0] = string_TO_ofxJSValue( name );
        	    args[1] = int_TO_ofxJSValue( valToggle ? 1 : 0 );
				ofxJSCallFunctionNameObject_IfExists(mp_obj,"eventUI", args,2,retVal);
			}
		}
	}
}

//--------------------------------------------------------------
void Animation::saveProperties(string id)
{
    if (mp_UIcanvas)
    {
        mp_UIcanvas->saveSettings( getPropertiesFilename(id) );
    }
}


//--------------------------------------------------------------
void Animation::loadProperties(string id)
{
    if (mp_UIcanvas)
    {
        mp_UIcanvas->loadSettings( getPropertiesFilename(id) );
    }
			//m_soundPlayer.print(m_name);
}

//--------------------------------------------------------------
string Animation::getPropertiesFilename(string id, bool isExtension)
{
    return "Config/animations/"+id+"_"+this->m_name+ ( isExtension ? ".xml" : "");
}


//--------------------------------------------------------------
ofxJSBOOL Animation::jsNewSlider(ofxJSContext* cx, ofxJSObject* obj, uintN argc, ofxJSValue* argv, ofxJSValue* retVal)
{
    
    Animation* pThis = sm_mapJSObj_Anim[obj];
//    printf(">>> obj=%p, pThis=%p, from the map %p\n", obj, pThis, sm_mapJSObj_Anim[obj]);
    if (pThis && pThis->mp_UIcanvas)
    {
        if (argc == 4)
        {
            string name = ofxJSValue_TO_string(argv[0]);
            float min = ofxJSValue_TO_float(argv[1]);
            float max = ofxJSValue_TO_float(argv[2]);
            float val = ofxJSValue_TO_float(argv[3]);
            
            pThis->mp_UIcanvas->addWidgetDown( new ofxUISlider(name, min,max,val, ANIM_UI_WIDTH_DEFAULT,ANIM_UI_HEIGHT_DEFAULT) );
        
            return JS_TRUE;
            
        }
    }
    
    return JS_FALSE;
}

//--------------------------------------------------------------
ofxJSBOOL Animation::jsNewToggle(ofxJSContext* cx, ofxJSObject* obj, uintN argc, ofxJSValue* argv, ofxJSValue* retVal)
{
    
    Animation* pThis = sm_mapJSObj_Anim[obj];
//    printf(">>> obj=%p, pThis=%p, from the map %p\n", obj, pThis, sm_mapJSObj_Anim[obj]);
    if (pThis && pThis->mp_UIcanvas)
    {
        if (argc == 2)
        {
            string name = ofxJSValue_TO_string(argv[0]);
            int val = ofxJSValue_TO_int(argv[1]);
            
            pThis->mp_UIcanvas->addWidgetDown( new ofxUIToggle(name, val == 0 ? false : true, ANIM_UI_HEIGHT_DEFAULT,ANIM_UI_HEIGHT_DEFAULT) );
        
            return JS_TRUE;
        }
    }
    
    return JS_FALSE;
}

//void Animation::jsNewSlider(string name, float min, float max, float val)
//{
//}


//--------------------------------------------------------------
void Animation::M_beginShader()
{
	m_shader.begin();
}

//--------------------------------------------------------------
void Animation::M_endShader()
{
	m_shader.end();
}

//--------------------------------------------------------------
void Animation::M_loadShader(string name)
{
	string v = "Shaders/"+name+".vert";
	string f = "Shaders/"+name+".frag";
	m_shader.load(v,f);
}



//--------------------------------------------------------------
void Animation::VM_enter()
{
	if (M_reloadScript())
	{
		ofxJSValue retVal;
		if (mp_obj){
			ofxJSCallFunctionNameObject_NoArgs_IfExists(mp_obj, "setup", retVal);
		}
	}
}

//--------------------------------------------------------------
void Animation::VM_exit()
{
	ofxJSValue retVal;
	if (mp_obj){
		ofxJSCallFunctionNameObject_NoArgs_IfExists(mp_obj, "exit", retVal);
	}

	M_deleteScript();	
}

//--------------------------------------------------------------
void Animation::M_deleteScript()
{
	if (mp_script){
		delete mp_script;
		mp_script=0;
	}
}

//--------------------------------------------------------------
void Animation::VM_update(float dt)
{
	if (mp_script)
	{
		// Reload
		if (Globals::instance()->m_isAutoReloadScript){
			m_tReloadScript += dt;
			if (m_tReloadScript>=1.0f){
				//M_reloadScript();
				// Check time stamp of file
				ofFile fileScript(m_pathAbsScript);
				Poco::File& pocoFileScript = fileScript.getPocoFile();
				Poco::Timestamp timestamp = pocoFileScript.getLastModified();
				if (timestamp > m_timestampFileScript){
					m_timestampFileScript = timestamp;
					M_reloadScript();
				}
			}
		}

		// Call update
		ofxJSValue retVal;
		ofxJSValue args[1];
		args[0] = float_TO_ofxJSValue(dt);
//		ofxJSCallFunctionNameGlobal_IfExists("update", args,1,retVal);

		if (mp_obj){
			ofxJSCallFunctionNameObject_IfExists(mp_obj,"update", args,1,retVal);
		}

	}
}

//--------------------------------------------------------------
void Animation::VM_draw(float w, float h)
{
	if (mp_script){
		ofxJSValue retVal;
		ofxJSValue args[2];
		args[0] = float_TO_ofxJSValue(w);
		args[1] = float_TO_ofxJSValue((h+1));
//		ofxJSCallFunctionNameGlobal_IfExists("draw", args,2,retVal);
		if (mp_obj){
			ofxJSCallFunctionNameObject_IfExists(mp_obj,"draw", args,2,retVal);
		}

	}
}

//--------------------------------------------------------------
void Animation::VM_doTransition(int which, float t)
{
//  SoundManager::instance()->setVolumeAll(t);
    
  if (mp_script && mp_obj)
  {
      ofxJSValue retVal;
      ofxJSValue args[2];
      args[0] = int_TO_ofxJSValue(which);
      args[1] = float_TO_ofxJSValue(t);

      ofxJSCallFunctionNameObject_IfExists(mp_obj,"doTransition", args,2,retVal);
  }
}

//--------------------------------------------------------------
void Animation::sOnVolumAccumEvent(void* pData, VolumeAccum* pVolumAccum)
{
	Animation* pThis = (Animation*)pData;
	if (pThis && pVolumAccum){
		 pThis->onVolumAccumEvent(pVolumAccum->m_deviceId);
	}
}

//--------------------------------------------------------------
void Animation::onVolumAccumEvent(string deviceId)
{
	playSound(deviceId);
}

//--------------------------------------------------------------
void Animation::accumulateVolume(float volume, string deviceId)
{
	// Volume Accumulators
	map<string,VolumeAccum*>::iterator it = m_mapDeviceVolumAccum.find(deviceId);
	VolumeAccum* pVolumeAccum = 0;
	if ( it == m_mapDeviceVolumAccum.end())
	{
		pVolumeAccum = new VolumeAccum();
		pVolumeAccum->m_deviceId = deviceId;
		pVolumeAccum->setTriggerInCb( sOnVolumAccumEvent, (void*) this);
		m_mapDeviceVolumAccum[deviceId] = pVolumeAccum;
		
		ofLog() << "creating volumeAccum instance for "<<deviceId;
	}
	else{
		pVolumeAccum = it->second;
	}
	
	if (pVolumeAccum)
		pVolumeAccum->add( volume );
}


//--------------------------------------------------------------
void Animation::onNewPacket(DevicePacket* pPacket, string deviceId, float xNorm, float yNorm)
{
	// Scripting
	if (mp_script && mp_obj && pPacket)
    {
		ofxJSValue retVal;
		ofxJSValue args[4];
		args[0] = string_TO_ofxJSValue(deviceId);
		args[1] = float_TO_ofxJSValue(pPacket->m_volume);
		args[2] = float_TO_ofxJSValue(xNorm);
		args[3] = float_TO_ofxJSValue(yNorm);
        ofxJSCallFunctionNameObject_IfExists(mp_obj,"onNewPacket", args,4,retVal);
    }
}

#define ____________AnimationManager____________

//--------------------------------------------------------------
AnimationManager::AnimationManager()
{
	mp_animationCurrent = 0;	
	mp_animationNext	= 0;
	m_state				= STATE_PLAY;
	m_transitionDuration = 2.0f;
    m_animationInfoIndex = 0;
	//m_transitionTween.setFrameBasedAnimation(true);

}

//--------------------------------------------------------------
AnimationManager::~AnimationManager()
{
	vector<Animation*>::iterator it;
	for (it = m_listAnimations.begin(); it != m_listAnimations.end(); ++it){
		delete *it;
	}
}

//--------------------------------------------------------------
void AnimationManager::M_setAnimation(int index, string args)
{
	Animation* pAnimation = M_getAnimation(index);
	M_setAnimation(pAnimation, args);
}

//--------------------------------------------------------------
void AnimationManager::M_setAnimation(string name, string args)
{
	vector<Animation*>::iterator it;
	for (it = m_listAnimations.begin(); it != m_listAnimations.end(); ++it){
		if (name == (*it)->m_name){
			M_setAnimation(*it,args);
			break;
		}
	}
}

//--------------------------------------------------------------
void AnimationManager::M_setAnimation(Animation* pAnimation, string args)
{
	mp_animationNext = pAnimation;
	m_animationNextArgs = args;
	M_changeState(STATE_TRANSITION_OUT);
}

//--------------------------------------------------------------
void AnimationManager::M_setAnimationDirect(string name, string args)
{
	vector<Animation*>::iterator it;
	for (it = m_listAnimations.begin(); it != m_listAnimations.end(); ++it){
		if (name == (*it)->m_name){
			M_changeAnimation(*it, args);
			break;
		}
	}
}

//--------------------------------------------------------------
bool AnimationManager::M_isTransiting()
{
    return (m_state == STATE_TRANSITION_IN || m_state == STATE_TRANSITION_OUT);
}


//--------------------------------------------------------------
void AnimationManager::M_changeAnimation(Animation* pAnimationNext, string args)
{
    
	if (pAnimationNext)
	{
		if (mp_animationCurrent)
			mp_animationCurrent->VM_exit();

        //SoundManager::instance()->stopAll();

		mp_animationCurrent = pAnimationNext;
		mp_animationCurrent->VM_setArgs(args);
		mp_animationCurrent->VM_enter();
	}
}

//--------------------------------------------------------------
void AnimationManager::setAnimationNext()
{
    int indexAnimation = getAnimationIndex( mp_animationCurrent );
    if (indexAnimation>=0)
    {
        int nbAnimations = m_listAnimations.size();
        M_setAnimation( m_listAnimations[(indexAnimation+1)%nbAnimations] );
    }
}


//--------------------------------------------------------------
void AnimationManager::setAnimationTimelineNext()
{
    // Get animation info
    AnimationInfo animInfo = m_listAnimationsInfos[m_animationInfoIndex];

    // Launch animation
    Animation* pAnimation = M_getAnimationByName( animInfo.m_name );
    if (pAnimation)
        M_setAnimation( pAnimation );

    // For next time
    int nbAnimationsInfo = m_listAnimationsInfos.size();
    m_animationInfoIndex = (m_animationInfoIndex+1)%nbAnimationsInfo;
}

//--------------------------------------------------------------
void AnimationManager::gotoAnimationNext()
{
    int indexAnimation = getAnimationIndex( mp_animationCurrent );
    if (indexAnimation>=0)
    {
        int nbAnimations = m_listAnimations.size();
        Animation* animationNext = m_listAnimations[(indexAnimation+1)%nbAnimations];
        M_changeAnimation(animationNext,"");
    }
}

//--------------------------------------------------------------
void AnimationManager::gotoAnimationPrev()
{
    int indexAnimation = getAnimationIndex( mp_animationCurrent );
    if (indexAnimation>=0)
    {
        int nbAnimations = m_listAnimations.size();
        if (indexAnimation==0)
        {
            indexAnimation = nbAnimations;
        }
        Animation* animationPrev = m_listAnimations[indexAnimation-1];
        M_changeAnimation(animationPrev,"");
    }
}

//--------------------------------------------------------------
int AnimationManager::getAnimationIndex(Animation* pAnimation)
{
    for (int i=0; i<m_listAnimations.size(); i++)
		if (m_listAnimations[i] == pAnimation)
            return i;
    return -1;
}

//--------------------------------------------------------------
Animation* AnimationManager::M_getAnimation(int index)
{
	if (index < m_listAnimations.size())
		return m_listAnimations[index];
	return 0;
}

//--------------------------------------------------------------
Animation* AnimationManager::M_getAnimationByName(string name)
{
	vector<Animation*>::iterator it;
	for (it = m_listAnimations.begin(); it != m_listAnimations.end(); ++it){
		if ((*it)->m_name == name)
			return *it;
	}	
	return 0;
}

//--------------------------------------------------------------
Animation* AnimationManager::M_getAnimationByThemeRnd(int theme)
{
	int nb = m_listAnimationsTheme[theme]->size();
	if (nb>0)
	{
		int rnd = (int)ofRandom(0,nb);
		return m_listAnimationsTheme[theme]->at(rnd); 
	}	


/*	
	vector<Animation*>::iterator it;
	for (it = m_listAnimations.begin(); it != m_listAnimations.end(); ++it){
		if ((*it)->m_name == name)
			return *it;
	}	
	return 0;
*/
	return 0;
}


//--------------------------------------------------------------
void AnimationManager::M_addAnimation(Animation* pAnimation)
{
	m_listAnimations.push_back(pAnimation);
}

//--------------------------------------------------------------
string AnimationManager::M_getAnimationNameRnd()
{
	int index = (int)ofRandom(m_listAnimations.size()-1);
	return m_listAnimations[index]->m_name;
}

//--------------------------------------------------------------
bool AnimationManager::M_isAutoClear()
{
	if (mp_animationCurrent)
		return mp_animationCurrent->m_isAutoClear;
	return true;
}

//--------------------------------------------------------------
void AnimationManager::M_update(float dt)
{
	// Update current animation
	if (mp_animationCurrent)
		mp_animationCurrent->VM_update(dt);
		
		
	// Update transition tween
	if (m_state == STATE_TRANSITION_OUT || m_state == STATE_TRANSITION_IN)
    {
		m_transitionTween.update();
        if (mp_animationCurrent)
            mp_animationCurrent->VM_doTransition(m_state, m_transitionTween.getTarget(0));
    }
    
	// tween end ? 
	if (m_transitionTween.isCompleted())
	{
//        SoundManager::instance()->stopAll();
        
		if (m_state == STATE_TRANSITION_OUT) 
			M_changeState(STATE_TRANSITION_IN);
		else
		if (m_state == STATE_TRANSITION_IN) 
			M_changeState(STATE_PLAY);
	}
}

//--------------------------------------------------------------
void AnimationManager::M_drawCanvas(float w, float h)
{
	// Draw animation
    ofPushStyle();
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
	if (mp_animationCurrent)
		mp_animationCurrent->VM_draw(w,h);
    M_drawTransition(w,h);
    ofDisableBlendMode();
    ofPopStyle();
}


//--------------------------------------------------------------
void AnimationManager::M_drawTransition(float w, float h)
{
	// Draw transition
	if (m_state == STATE_TRANSITION_OUT || m_state == STATE_TRANSITION_IN)
	{
		//printf("tween=%.2f\n", m_transitionTween.getTarget(0));
	
		ofPushStyle();
			ofSetColor(0,0,0, 255*m_transitionTween.getTarget(0));
			ofRect(0,0,w,h);
		ofPopStyle();
	}
}

//--------------------------------------------------------------
bool AnimationManager::M_editScript()
{
	if (mp_animationCurrent && mp_animationCurrent->M_isScript()){
		ofLVOpenProgram(mp_animationCurrent->m_pathAbsScript);
	}
}

//--------------------------------------------------------------
bool AnimationManager::M_reloadScript()
{
	if (mp_animationCurrent)
		return mp_animationCurrent->M_reloadScript();
}

//--------------------------------------------------------------
void AnimationManager::M_readSettings(ofxXmlSettings& settings)
{
    settings.pushTag("surface");
    settings.pushTag("timeline");
    
    
    int nbAnimations = settings.getNumTags("animation");
    printf("AnimationManager >>> nbAnimations=%d\n", nbAnimations);
    for (int i=0;i<nbAnimations;i++)
    {
        string animName = settings.getValue("animation","",i);
        
        if (animName != "" && M_getAnimationByName(animName) != 0){
            printf("- %s\n", animName.c_str());
            m_listAnimationsInfos.push_back( AnimationInfo(animName) );
        }
    }

    settings.popTag();
    settings.popTag();
}

//--------------------------------------------------------------
void AnimationManager::M_changeState(int newState)
{
	if (m_state == STATE_PLAY)
	{
		if (newState == STATE_TRANSITION_OUT)
		{
			m_transitionTween.setParameters(1, easinglinear, ofxTween::easeOut, 0, 1, m_transitionDuration*1000.0f, 0);
			m_transitionTween.start();
			m_state = newState;
			
			printf("STATE_PLAY > STATE_TRANSITION_OUT\n");
		}
	}
	else
	if (m_state == STATE_TRANSITION_OUT)
	{
		if (newState == STATE_TRANSITION_IN)
		{
			//if (mp_animationNext != mp_animationCurrent)
			if (mp_animationNext)
			{
				M_changeAnimation(mp_animationNext,m_animationNextArgs);
				mp_animationNext = 0;

				m_transitionTween.setParameters(1, easinglinear, ofxTween::easeOut, 1, 0, m_transitionDuration*1000.0f, 0);
				m_transitionTween.start();
			}
			m_state = newState;
		
			printf("STATE_TRANSITION_OUT > STATE_TRANSITION_IN\n");
		}
	}
	else
	if (m_state == STATE_TRANSITION_IN)
	{
		if (newState == STATE_PLAY)
		{
			m_state = newState;
			printf("STATE_TRANSITION_IN > STATE_PLAY\n");
		}
	}
}

