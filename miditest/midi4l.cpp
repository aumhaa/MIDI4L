#include <map>
#include "RtMidi.h"
#include "maxcpp6.h"

typedef std::map<t_symbol*,int> portmap;

const int MAX_STR_SIZE = 512;

const int OUTLET_MIDIIN   = 0;
const int OUTLET_INPORTS  = 1;
const int OUTLET_OUTPORTS = 2;

t_symbol *SYM_APPEND = gensym("append");
t_symbol *SYM_CLEAR  = gensym("clear");


class MIDI4L : public MaxCpp6<MIDI4L> {
    
public:
    
	MIDI4L(t_symbol * sym, long ac, t_atom * av) {
		setupIO(1, 3); // inlets / outlets
        
        try {
            midiin = new RtMidiIn();
        }
        catch ( RtMidiError &error ) {
            error("RtMidiIn constructor failure");
            printError(error);
        }
        
        try {
            midiout = new RtMidiOut();
        }
        catch ( RtMidiError &error ) {
            error("RtMidiOut consturctor failure");
            printError(error);
        }
        
        refreshPorts();
        printPorts(); // TODO: comment this out before shipping
	}
	
    MIDI4L() {
        if(midiin)  delete midiin;
        if(midiout) delete midiout;
    }
    
    
    /**
     * Setup inlet and outlet assistance messages.
     */
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
    
    
    /**
     * List all input and output ports.
     * Also refreshes the port list in case a device was plugged in or unplugged.
     */
	void bang(long inlet) {
        refreshPorts();
        dumpPorts(m_outlets[OUTLET_INPORTS],  inPortMap);
        dumpPorts(m_outlets[OUTLET_OUTPORTS], outPortMap);
    }
    
    
    /**
     * Set the input port by name.
     * If the name is valid, this object will start sending messages out it's outlet when MIDI is received.
     * If the name is invalid, an error is printed to the Max console and nothing else happens.
     */
    void inport(long inlet, t_symbol *s, long ac, t_atom *av) {
        t_symbol *portName = _sym_nothing;
        
        if( atom_arg_getsym(&portName, 0, ac, av) == MAX_ERR_NONE ) {
            post("Got inport name %s", *portName);
            int portIndex = getPortIndex(inPortMap, portName);
            if (portIndex >= 0) {
                post("Found port at index %i", portIndex);
                openMidiInput(portIndex);
            }
        }
        else {
            error("Invalid inport message. A portname is required");
        }
    }
    
    
    /**
     * Set the output port by name.
     * If the name is valid, this object will pass messages received to it's first inlet to the MIDI port.
     * If the name is invalid, an error is printed to the Max console and nothing else happens.
     */
    void outport(long inlet, t_symbol *s, long ac, t_atom *av) {
        t_symbol *portName = _sym_nothing;
        
        if( atom_arg_getsym(&portName, 0, ac, av) == MAX_ERR_NONE ) {
            post("Got outport name %s", *portName);
            int portIndex = getPortIndex(outPortMap, portName);
            if (portIndex >= 0) {
                post("Found port at index %i", portIndex);
                // TODO: now hook up the port
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
    portmap inPortMap;
    portmap outPortMap;
    
    
    /**
     * Get the list of available ports. 
     * Can be called repeatedly to regenerate the list if a MIDI device is plugged in or unplugged.
     */
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
                    printError(error);                }
            }
        }
        
        if(midiout) {
            numOutPorts = midiout->getPortCount();
            for ( int i=0; i<numOutPorts; i++ ) {
                try {
                    std::string portName = midiout->getPortName(i);
                    strncpy(cPortName, portName.c_str(), MAX_STR_SIZE);
                    cPortName[MAX_STR_SIZE - 1] = NULL;
                    
                    outPortMap[gensym(cPortName)] = i;
                }
                catch (RtMidiError &error) {
                    error("Error getting MIDI output port name");
                    printError(error);
                }
            }
        }
    }

    
    /**
     * Lookup a port index given a portMap (input or output port map) and a portName.
     * Returns the port index, or -1 if the port is not found
     */
    int getPortIndex(portmap portMap, t_symbol *portName) {
        portmap::iterator iter = portMap.find(portName);
        if (iter == portMap.end()) {
            error("Port not found: %s", *portName);
            return -1;
        }
        else {
            return iter->second;
        }
    }
    
    
    void openMidiInput(int portIndex) {
        midiin->openPort( portIndex );
    
        // TODO: not sure how to get this to compile
        //midiin->setCallback( &(MIDI4L::midiInputCallback) );
        
        // Don't ignore sysex, timing, or active sensing messages.
        // Does Max ignore timing or active sensing message?
        midiin->ignoreTypes( false, false, false );
        post("Reading MIDI input");
    }
    
    
    void midiInputCallback( double deltatime, std::vector< unsigned char > *message, void *userData )
    {
        int nBytes = message->size();
        for ( int i=0; i<nBytes; i++ ) {
            int byte = (int)message->at(i);
            post("Byte %i: %i", i, byte);
        }
    }
    
    
    /**
     * Print the port names to the Max console
     */
    void printPorts() {
        post("MIDI input Port count: %u", numInPorts);
        
        for( portmap::iterator iter=inPortMap.begin(); iter!=inPortMap.end(); iter++ ) {
            t_symbol *portName = (*iter).first;
            post("input %u: %s", (*iter).second, *portName);
        }
        
        post(" ");
        post("MIDI output port count: %u", numOutPorts);
        
        for( portmap::iterator iter=outPortMap.begin(); iter!=outPortMap.end(); iter++ ) {
            t_symbol* portName = (*iter).first;
            post("output %u: %s", (*iter).second, *portName);
        }
    }
    
    
    /**
     * Send messages to build a umenus for the input (2nd outlet) and output (3rd outlet) ports
     */
    void dumpPorts(void *outlet, portmap portMap) {
        t_atom atoms[1];
        
        outlet_anything(outlet, SYM_CLEAR, 0, NULL);
        
        for( portmap::iterator iter=portMap.begin(); iter!=portMap.end(); iter++ ) {
            t_symbol *portName = (*iter).first;
            atom_setsym(&atoms[0], portName);
            outlet_anything(outlet, SYM_APPEND, 1, atoms);
        }
    }
    
    
    /**
     * Print an RtMidiError to the Max console
     * NOTE: I have not been able to test this code because I don't know how to trigger an RtMidiError. It always "just works" for me.
     */
    void printError(RtMidiError &error) {
        std::string msg = error.getMessage();
        char cstr[MAX_STR_SIZE];
        strncpy(cstr, msg.c_str(), MAX_STR_SIZE);
        cstr[MAX_STR_SIZE - 1] = NULL;
        error(cstr);
    }
};


C74_EXPORT int main(void) {
	MIDI4L::makeMaxClass("midi4l");
    REGISTER_METHOD_ASSIST(MIDI4L, assist);
    REGISTER_METHOD(MIDI4L, bang);
    REGISTER_METHOD_GIMME(MIDI4L, inport);
    REGISTER_METHOD_GIMME(MIDI4L, outport);
}
