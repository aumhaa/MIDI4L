#include <map>
#include "RtMidi.h"
#include "maxcpp6.h"

const int MAX_PORT_NAME_SIZE = 1024;

class Example : public MaxCpp6<Example> {
public:
	Example(t_symbol * sym, long ac, t_atom * av) { 
		setupIO(2, 2); // inlets / outlets
	}
	~Example() {}	
	
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
};

C74_EXPORT int main(void) {
	// create a class with the given name:
	Example::makeMaxClass("example");
	REGISTER_METHOD(Example, bang);
	REGISTER_METHOD_FLOAT(Example, testfloat);
	REGISTER_METHOD_LONG(Example, testint);
	REGISTER_METHOD_GIMME(Example, test);

    std::string portName;
    char cPortName[MAX_PORT_NAME_SIZE];

    std::map<std::string, int> inPortMap;
    std::map<std::string, int> outPortMap;
    
	RtMidiIn  *midiin = 0;
	RtMidiOut *midiout = 0;
	// RtMidiIn constructor
	try {
		midiin = new RtMidiIn();
	}
	catch ( RtMidiError &error ) {
		error.printMessage();
		post("MIDIin exit failure");
		exit( EXIT_FAILURE ); // TODO: probably shouldn't call exit() in a Max external. Log an error to Max console and skip logic below as necessary.
	}
    
	// Check inputs.
	unsigned int nPorts = midiin->getPortCount();
	// std::cout << "\nThere are " << nPorts << " MIDI input sources available.\n";
	post("InPort count: %u", nPorts);
	for ( unsigned int i=0; i<nPorts; i++ ) {
		try {
			portName = midiin->getPortName(i);
            strncpy(cPortName, portName.c_str(), MAX_PORT_NAME_SIZE);
            cPortName[MAX_PORT_NAME_SIZE - 1] = NULL;
			post("inport %u: %s", i, cPortName);
		}
		catch ( RtMidiError &error ) {
			error.printMessage();
			goto cleanup;
		}
		// std::cout << "  Input Port #" << i+1 << ": " << portName << '\n';
	}
    
	// RtMidiOut constructor
	try {
		midiout = new RtMidiOut();
	}
	catch ( RtMidiError &error ) {
		error.printMessage();
		exit( EXIT_FAILURE );  // TODO: probably shouldn't call exit() in a Max external. Log an error to Max console and skip logic below as necessary.
	}
    
    post("");
    
	// Check outputs.
	nPorts = midiout->getPortCount();
	// std::cout << "\nThere are " << nPorts << " MIDI output ports available.\n";
	post("Outport count: %u", nPorts);
	for ( unsigned int i=0; i<nPorts; i++ ) {
		try {
			portName = midiout->getPortName(i);
            strncpy(cPortName, portName.c_str(), MAX_PORT_NAME_SIZE);
            cPortName[MAX_PORT_NAME_SIZE - 1] = NULL;
            post("outport %u: %s", i, cPortName);
		}
		catch (RtMidiError &error) {
			error.printMessage();
			post("MIDIout exit failure");
			goto cleanup;
		}
		// std::cout << "  Output Port #" << i+1 << ": " << portName << '\n';
	}
	// std::cout << '\n';
    
	// Clean up
cleanup:
	delete midiin;
	delete midiout;
	return 0;
}