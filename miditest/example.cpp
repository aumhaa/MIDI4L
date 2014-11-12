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
        printPorts(); // TODO: comment this out before shipping
	}
	
    ~Example() {
        if(midiin)  delete midiin;
        if(midiout) delete midiout;
    }
    
    
    void assist(void *b, long io, long index, char *msg) {
        if (io == ASSIST_INLET) {
            strncpy_zero(msg, "send MIDI output (list), list ports (bang), set input port (inport name), set output port (outport name)", MAX_STR_SIZE);
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
    
    void inport(long inlet, t_symbol *s, long ac, t_atom *av) {
        t_symbol *portName = _sym_nothing;
        
        if( atom_arg_getsym(&portName, 0, ac, av) == MAX_ERR_NONE ) {
            post("Got inport name %s", *portName);
            int portIndex = getPortIndex(inPortMap, portName);
            if (portIndex >= 0) {
                post("Found port at index %i", portIndex);
            }
        }
        else {
            error("Invalid inport message. A portname is required");
        }
    }
    
    void outport(long inlet, t_symbol *s, long ac, t_atom *av) {
        t_symbol *portName = _sym_nothing;
        
        if( atom_arg_getsym(&portName, 0, ac, av) == MAX_ERR_NONE ) {
            post("Got outport name %s", *portName);
            int portIndex = getPortIndex(outPortMap, portName);
            if (portIndex >= 0) {
                post("Found port at index %i", portIndex);
            }
        }
        else {
            error("Invalid outport message. A portname is required");
        }
    }
    
    
private:
    RtMidiIn  *midiin  = NULL;
    RtMidiOut *midiout = NULL;
    int numInPorts  = -1;
    int numOutPorts = -1;
    std::map<t_symbol *, int> inPortMap;
    std::map<t_symbol *, int> outPortMap;
    
    
    // Get the list of available ports. Can be called repeatedly to regenerate the list if a MIDI device is plugged in or unplugged.
    void refreshPorts() {
        char cPortName[MAX_STR_SIZE];
        
        inPortMap.clear();
        outPortMap.clear();

        if(midiin) {
            numInPorts = midiin->getPortCount();
            for ( int i=0; i<numInPorts; i++ ) {
                try {
                    std::string portName = midiin->getPortName(i);
                    strncpy(cPortName, portName.c_str(), MAX_STR_SIZE);
                    cPortName[MAX_STR_SIZE - 1] = NULL;
                    
                    inPortMap[gensym(cPortName)] = i;
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
                    std::string portName = midiout->getPortName(i);
                    strncpy(cPortName, portName.c_str(), MAX_STR_SIZE);
                    cPortName[MAX_STR_SIZE - 1] = NULL;
                    
                    // TODO: not sure if we should store the pointers or what they reference?
                    outPortMap[gensym(cPortName)] = i;
                }
                catch (RtMidiError &error) {
                    error("Error getting MIDI output port name");
                    error.printMessage(); // TODO: print this to the Max console
                }
            }
        }
    }

    
    // Returns the port index for the given portName in the given portMap, or -1 if the port is not found
    int getPortIndex(std::map<t_symbol*,int> portMap, t_symbol *portName) {
        std::map<t_symbol*,int>::iterator iter = portMap.find(portName);
        if (iter == portMap.end()) {
            error("Port not found: %s", *portName);
            return -1;
        }
        else {
            return iter->second;
        }
    }
    
    
    // Print the port names to the Max console
    void printPorts() {
        post("MIDI input Port count: %u", numInPorts);
        
        for( std::map<t_symbol*,int>::iterator iter=inPortMap.begin(); iter!=inPortMap.end(); iter++ ) {
            t_symbol *portName = (*iter).first;
            post("input %u: %s", (*iter).second, *portName);
        }
        
        post(" ");
        post("MIDI output port count: %u", numOutPorts);
        
        for( std::map<t_symbol*,int>::iterator iter=outPortMap.begin(); iter!=outPortMap.end(); iter++ ) {
            t_symbol* portName = (*iter).first;
            post("output %u: %s", (*iter).second, *portName);
        }
    }
    
    
    void dumpPorts(void *outlet, std::map<t_symbol*,int> portMap) {
        t_symbol *appendSym = gensym("append"); // make a constant? and for clear?
        t_atom atoms[1];
        
        outlet_anything(outlet, gensym("clear"), 0, NULL);
        
        for( std::map<t_symbol*,int>::iterator iter=portMap.begin(); iter!=portMap.end(); iter++ ) {
            t_symbol *portName = (*iter).first;
            atom_setsym(&atoms[0], portName);
            outlet_anything(outlet, appendSym, 1, atoms);
        }
    }
};


C74_EXPORT int main(void) {
	Example::makeMaxClass("example");
    REGISTER_METHOD_ASSIST(Example, assist);
    REGISTER_METHOD(Example, bang);
    REGISTER_METHOD_GIMME(Example, inport);
    REGISTER_METHOD_GIMME(Example, outport);
}