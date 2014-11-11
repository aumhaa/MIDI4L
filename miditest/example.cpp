#include <map>
#include "RtMidi.h"
#include "maxcpp6.h"

const int MAX_PORT_NAME_SIZE = 1024;


class Example : public MaxCpp6<Example> {
    
public:
	Example(t_symbol * sym, long ac, t_atom * av) { 
		setupIO(2, 3); // inlets / outlets
        
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
        
        // TODO: maybe only do the following print-outs if a verbose attr is set, or a DEBUG flag is enabled during dev
        printPorts();
	}
	
    ~Example() {
        if(midiin)  delete midiin;
        if(midiout) delete midiout;
    }
	
    
	void bang(long inlet) {
        t_symbol *appendSym = gensym("append"); // make a constant? and for clear?
        char cPortName[MAX_PORT_NAME_SIZE];
        void *dumpOutlet = m_outlets[1]; // TODO: make a constant for this
        t_atom portNameAtoms[1];
        
        outlet_anything(dumpOutlet, gensym("clear"), 0, NULL);
        
        // TODO: need to switch between inPortMap or outPortMap depending on whether we're in input or output mode
        std::map<std::string,int> portMap = inPortMap;
        
        for( std::map<std::string,int>::iterator iter=portMap.begin(); iter!=portMap.end(); iter++ ) {
            std::string portName = (*iter).first;
            strncpy(cPortName, portName.c_str(), MAX_PORT_NAME_SIZE);
            cPortName[MAX_PORT_NAME_SIZE - 1] = NULL;

            atom_setsym(&portNameAtoms[0], gensym(cPortName));
            
            outlet_anything(dumpOutlet, appendSym, 1, portNameAtoms);
        }
	}
    
    
    // Refresh the port list when a MIDI device is plugged in or unplugged.
    // TODO: need to test what happens when we've already opened a port for a device that is then unplugged
    void refresh(long inlet, t_symbol * s, long ac, t_atom * av) {
        refreshPorts();
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
    }

    
    
    // Print the port names to the Max console
    void printPorts() {
        char cPortName[MAX_PORT_NAME_SIZE];
        
        post("MIDI input Port count: %u", numInPorts);
        
        for( std::map<std::string,int>::iterator iter=inPortMap.begin(); iter!=inPortMap.end(); iter++ ) {
            std::string portName = (*iter).first;
            strncpy(cPortName, portName.c_str(), MAX_PORT_NAME_SIZE);
            cPortName[MAX_PORT_NAME_SIZE - 1] = NULL;
            post("input %u: %s", (*iter).second, cPortName);
        }
        
        post(" ");
        post("MIDI output port count: %u", numOutPorts);
        
        for( std::map<std::string,int>::iterator iter=outPortMap.begin(); iter!=outPortMap.end(); iter++ ) {
            std::string portName = (*iter).first;
            strncpy(cPortName, portName.c_str(), MAX_PORT_NAME_SIZE);
            cPortName[MAX_PORT_NAME_SIZE - 1] = NULL;
            post("output %u: %s", (*iter).second, cPortName);
        }
    }
};


C74_EXPORT int main(void) {
	Example::makeMaxClass("example");
	REGISTER_METHOD(Example, bang);
    REGISTER_METHOD_GIMME(Example, refresh);
}