#include <map>
#include "RtMidi.h"
#include "maxcpp6.h"

const int MAX_PORT_NAME_SIZE = 1024;


class Example : public MaxCpp6<Example> {
    
public:
	Example(t_symbol * sym, long ac, t_atom * av) { 
		setupIO(2, 2); // inlets / outlets
        
        initRtMidi();
        
        // TODO: maybe only do the following print-outs if a verbose attr is set, or a DEBUG flag is enabled during dev
        printPorts();
	}
	
    ~Example() {
        if(midiin)  delete midiin;
        if(midiout) delete midiout;
    }
	
    
	// methods:
	void bang(long inlet) { 
		outlet_bang(m_outlets[0]);
	}
    
    
	void testfloat(long inlet, double v) { 
		outlet_float(m_outlets[0], v);
	}
    
    
	void testint(long inlet, long v) { 
		outlet_int(m_outlets[0], v);
	}
    
    
	void test(long inlet, t_symbol * s, long ac, t_atom * av) { 
		outlet_anything(m_outlets[1], gensym("test"), ac, av); 
	}
    
    
private:
    RtMidiIn*  midiin  = NULL;
    RtMidiOut* midiout = NULL;
    int numInPorts  = -1;
    int numOutPorts = -1;
    std::map<std::string, int> inPortMap;
    std::map<std::string, int> outPortMap;
    
    
    void initRtMidi() {
        std::string portName;
        
        try {
            midiin = new RtMidiIn();
        }
        catch ( RtMidiError &error ) {
            error("RtMidiIn constructor failure");
            error.printMessage(); // TODO: print this to the Max console
            return;
        }
        
        try {
            midiout = new RtMidiOut();
        }
        catch ( RtMidiError &error ) {
            error("RtMidiOut consturctor failure");
            error.printMessage(); // TODO: print this to the Max console
            return;
        }
        
        numInPorts = midiin->getPortCount();
        for ( int i=0; i<numInPorts; i++ ) {
            try {
                portName = midiin->getPortName(i);
                inPortMap[portName] = i;
            }
            catch ( RtMidiError &error ) {
                error("Error getting MIDI input port name");
                error.printMessage(); // TODO: print this to the Max console
            }
            
        }
        
        numOutPorts = midiout->getPortCount();
        for ( int i=0; i<numOutPorts; i++ ) {
            try {
                portName = midiout->getPortName(i);
                outPortMap[portName] = i;
            }
            catch (RtMidiError &error) {
                error("Error getting MIDI output port name");
                error.printMessage(); // TODO: print this to the Max console
            }
        }
    }
    
    
    void printPorts() {
        std::string portName;
        char cPortName[MAX_PORT_NAME_SIZE];
        
        post("MIDI input Port count: %u", numInPorts);
        for( std::map<std::string,int>::iterator iter=inPortMap.begin(); iter!=inPortMap.end(); iter++ ) {
            portName = (*iter).first;
            strncpy(cPortName, portName.c_str(), MAX_PORT_NAME_SIZE);
            cPortName[MAX_PORT_NAME_SIZE - 1] = NULL;
            post("input %u: %s", (*iter).second, cPortName);
        }
        
        post(" ");
        
        post("MIDI output port count: %u", numOutPorts);
        for( std::map<std::string,int>::iterator iter=outPortMap.begin(); iter!=outPortMap.end(); iter++ ) {
            portName = (*iter).first;
            strncpy(cPortName, portName.c_str(), MAX_PORT_NAME_SIZE);
            cPortName[MAX_PORT_NAME_SIZE - 1] = NULL;
            post("output %u: %s", (*iter).second, cPortName);
        }
    }
};


C74_EXPORT int main(void) {
	Example::makeMaxClass("example");
	REGISTER_METHOD(Example, bang);
	REGISTER_METHOD_FLOAT(Example, testfloat);
	REGISTER_METHOD_LONG(Example, testint);
	REGISTER_METHOD_GIMME(Example, test);
}