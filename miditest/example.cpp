#include <map>
#include "RtMidi.h"
#include "maxcpp6.h"

const int MAX_STR_SIZE = 512;

const int OUTLET_MIDIIN   = 0;
const int OUTLET_INPORTS  = 1;
const int OUTLET_OUTPORTS = 2;


class Example : public MaxCpp6<Example> {
    
public:
	Example(t_symbol * sym, long ac, t_atom * av) { 
		setupIO(1, 3); // inlets / outlets
        
        try {
            midiin = new RtMidiIn();
        }
        catch ( RtMidiError &error ) {
            error("RtMidiIn constructor failure");
            error.printMessage(); // TODO: print this to the Max console
        }
        
        try {
            midiout = new RtMidiOut();
        }
        catch ( RtMidiError &error ) {
            error("RtMidiOut consturctor failure");
            error.printMessage(); // TODO: print this to the Max console
        }
        
        refreshPorts();
	}
	
    ~Example() {
        if(midiin)  delete midiin;
        if(midiout) delete midiout;
    }
    
    
    void assist(void *b, long io, long index, char *msg) {
        if (io == ASSIST_INLET) {
            strncpy_zero(msg, "send MIDI output (list), list ports (bang), set port (port name)", MAX_STR_SIZE);
        }
        else if (io==ASSIST_OUTLET) {
            switch (index) {
                case OUTLET_MIDIIN:
                    strncpy_zero(msg, "receive MIDI input", MAX_STR_SIZE);
                    break;
                case OUTLET_INPORTS:
                    strncpy_zero(msg, "input port list", MAX_STR_SIZE);
                    break;
                case OUTLET_OUTPORTS:
                    strncpy_zero(msg, "output port list", MAX_STR_SIZE);
                    break;
            }
        }
    }
    
    
    // List all input and output ports. Also refreshes the port list in case a device was plugged in or unplugged.
	void bang(long inlet) {
        refreshPorts();
        dumpPorts(m_outlets[OUTLET_INPORTS],  inPortMap);
        dumpPorts(m_outlets[OUTLET_OUTPORTS], outPortMap);
    }
    
    
private:
    RtMidiIn  *midiin  = NULL;
    RtMidiOut *midiout = NULL;
    int numInPorts  = -1;
    int numOutPorts = -1;
    std::map<std::string, int> inPortMap;
    std::map<std::string, int> outPortMap;
    
    
    // Get the list of available ports. Can be called repeatedly to regenerate the list if a MIDI device is plugged in or unplugged.
    void refreshPorts() {
        std::string portName;
        inPortMap.clear();
        outPortMap.clear();

        if(midiin) {
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
        }
        
        if(midiout) {
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
        // printPorts();
    }

    
    
    // Print the port names to the Max console
    void printPorts() {
        char cPortName[MAX_STR_SIZE];
        
        post("MIDI input Port count: %u", numInPorts);
        
        for( std::map<std::string,int>::iterator iter=inPortMap.begin(); iter!=inPortMap.end(); iter++ ) {
            std::string portName = (*iter).first;
            strncpy(cPortName, portName.c_str(), MAX_STR_SIZE);
            cPortName[MAX_STR_SIZE - 1] = NULL;
            post("input %u: %s", (*iter).second, cPortName);
        }
        
        post(" ");
        post("MIDI output port count: %u", numOutPorts);
        
        for( std::map<std::string,int>::iterator iter=outPortMap.begin(); iter!=outPortMap.end(); iter++ ) {
            std::string portName = (*iter).first;
            strncpy(cPortName, portName.c_str(), MAX_STR_SIZE);
            cPortName[MAX_STR_SIZE - 1] = NULL;
            post("output %u: %s", (*iter).second, cPortName);
        }
    }
    
    
    void dumpPorts(void *outlet, std::map<std::string,int> portMap) {
        t_symbol *appendSym = gensym("append"); // make a constant? and for clear?
        char cPortName[MAX_STR_SIZE];
        t_atom portNameAtoms[1];
        
        outlet_anything(outlet, gensym("clear"), 0, NULL);
        
        for( std::map<std::string,int>::iterator iter=portMap.begin(); iter!=portMap.end(); iter++ ) {
            std::string portName = (*iter).first;
            strncpy(cPortName, portName.c_str(), MAX_STR_SIZE);
            cPortName[MAX_STR_SIZE - 1] = NULL;
            atom_setsym(&portNameAtoms[0], gensym(cPortName));
            
            outlet_anything(outlet, appendSym, 1, portNameAtoms);
        }
    }
};


C74_EXPORT int main(void) {
	Example::makeMaxClass("example");
	REGISTER_METHOD(Example, bang);
    
    REGISTER_METHOD_ASSIST(Example, assist);
}