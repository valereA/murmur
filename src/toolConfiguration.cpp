//
//  toolConfiguration.cpp
//  murmur
//
//  Created by Julien on 05/06/2014.
//
//

#include "toolConfiguration.h"
#include "globals.h"
#include "testApp.h"
#include "utils.h"

//--------------------------------------------------------------
toolConfiguration::toolConfiguration(toolManager* parent) : tool("Configuration", parent)
{
	isShowDevicePointSurfaces	= false;
	isViewSimulation			= true;
	m_isLaunchMadMapper			= false;
	m_isLaunchDevices			= false;

	mp_tgViewSimu				= 0;
	mp_tgFullscreen				= 0;
	m_isFullscreen				= false;
}

//--------------------------------------------------------------
toolConfiguration::~toolConfiguration()
{
	vector<threadRasp*>::iterator it;
	for (it = m_listThreadLaunchDevices.begin(); it!=m_listThreadLaunchDevices.end(); ++it)
	{
		(*it)->waitForThread(true);
		delete (*it);
	}
}
//--------------------------------------------------------------
void toolConfiguration::createControlsCustom()
{
	if (mp_canvas)
	{
		float dim = 16;
		int widthDefault = 320;

	    mp_canvas->addWidgetDown( new ofxUILabel("Configuration",OFX_UI_FONT_LARGE) );
    	mp_canvas->addWidgetDown(new ofxUISpacer(widthDefault, 2));

	    mp_canvas->addToggle("Show device points", isShowDevicePointSurfaces, dim, dim);
    	mp_tgViewSimu = mp_canvas->addToggle("View simulation", isViewSimulation, dim, dim);

    	mp_canvas->addWidgetDown(new ofxUIToggle("Launch madmapper @ start", false, dim, dim));
    	mp_canvas->addWidgetDown(new ofxUIToggle("Launch murmur @ rasp", false, dim, dim));

		mp_tgFullscreen = new ofxUIToggle("Fullscreen", false, dim, dim);
    	mp_canvas->addWidgetDown( mp_tgFullscreen );

    	mp_canvas->addWidgetDown(new ofxUISpacer(widthDefault, 2));
		mp_canvas->addWidgetDown(new ofxUILabelButton("Edit configuration.xml", 100, false, OFX_UI_FONT_SMALL));
		mp_canvas->addWidgetDown(new ofxUILabel("warning", "/!\\ Editing this file needs", OFX_UI_FONT_SMALL));
		mp_canvas->addWidgetDown(new ofxUILabel("warning", "   restarting the application", OFX_UI_FONT_SMALL));

		mp_canvas->autoSizeToFitWidgets();
		mp_canvas->setVisible(false);
	}
}

//--------------------------------------------------------------
void toolConfiguration::setup()
{
	if (!GLOBALS->mp_app->isSimulation)
	{
		if (m_isLaunchMadMapper)
		{
			launchMadMapper();
		}
	
		// Launch Murmur on Raspberry
		if (m_isLaunchDevices)
		{
			launchDevices();
		}
	}
}

//--------------------------------------------------------------
void toolConfiguration::launchDevices()
{
	printf("[testApp::launchDevices]\n");
	int nbLaunchDevices = GLOBALS->mp_app->m_settings.getNumTags("murmur:launchDevices:ip");
	for (int i=0;i<nbLaunchDevices;i++)
	{
		string strIPMurmur 	= GLOBALS->mp_app->m_settings.getValue("murmur:launchDevices:ip", "10.23.108.114", i);
		string strSSH_kill 	= "ssh pi@" + strIPMurmur + " 'sudo pkill -f murmurRaspberry'";
		string strSSH_run 	= "ssh pi@" + strIPMurmur + " /home/pi/Dev/C/openFrameworks/examples/myapps/murmurRaspberry/bin/run_murmur.sh";
		
		threadRasp* pThreadLaunchDevice = new threadRasp();
		pThreadLaunchDevice->addCommand(strSSH_kill);
		pThreadLaunchDevice->addCommand(strSSH_run);
		pThreadLaunchDevice->startThread();
		
		m_listThreadLaunchDevices.push_back(pThreadLaunchDevice);
	}
}

//--------------------------------------------------------------
void toolConfiguration::launchMadMapper()
{
	string pathFileMM = GLOBALS->mp_app->m_settings.getValue("murmur:madmapper", "default.map");
	ofFile file(ofToDataPath("Config/madmapper/"+pathFileMM));
	if (file.exists())
	{
		string command = "open "+file.getAbsolutePath();
		system(command.c_str());
	   printf("Launching MadMapper with %s\n", file.getAbsolutePath().c_str());
	}
	else
	{
	   printf("Error launching MadMapper with %s\n", file.getAbsolutePath().c_str());
	}
}

//--------------------------------------------------------------
void toolConfiguration::setFullscreen(bool is)
{
	m_isFullscreen = is;
	ofSetFullscreen(m_isFullscreen);
	updateUI();
}

//--------------------------------------------------------------
void toolConfiguration::toggleFullscreen()
{
	setFullscreen(!m_isFullscreen);
}

//--------------------------------------------------------------
void toolConfiguration::setViewSimulation(bool is)
{
	isViewSimulation = is;
	GLOBALS->mp_app->setViewSimulation(isViewSimulation);
	updateUI();
}

//--------------------------------------------------------------
void toolConfiguration::toggleViewSimulation()
{
	setViewSimulation(!isViewSimulation);
}


//--------------------------------------------------------------
void toolConfiguration::updateUI()
{
	if (mp_tgFullscreen)
		mp_tgFullscreen->setValue(m_isFullscreen);
	if (mp_tgViewSimu)
		mp_tgViewSimu->setValue(isViewSimulation);
}

//--------------------------------------------------------------
void toolConfiguration::handleEvents(ofxUIEventArgs& e)
{
    string name = e.widget->getName();

	if (name == "View simulation")
    {
        isViewSimulation = ((ofxUIToggle *) e.widget)->getValue();
		GLOBALS->mp_app->setViewSimulation( isViewSimulation );
    }
    else if (name == "Show device points")
    {
        GLOBALS->mp_app->isShowDevicePointSurfaces = ((ofxUIToggle *) e.widget)->getValue();
    }
	else if (name == "Launch madmapper @ start")
	{
		m_isLaunchMadMapper = ((ofxUIToggle *) e.widget)->getValue();
	}
	else
	if (name == "Launch murmur @ rasp")
	{
		m_isLaunchDevices = ((ofxUIToggle *) e.widget)->getValue();
	}
	else
	if (name == "Fullscreen")
	{
		m_isFullscreen = ((ofxUIToggle *) e.widget)->getValue();
		ofSetFullscreen( m_isFullscreen );
	}
	else
	if (name == "Edit configuration.xml")
	{
		ofLVOpenProgram(ofToDataPath("configuration.xml"));
	}
}
