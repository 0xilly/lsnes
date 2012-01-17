#ifndef _keymapper__hpp__included__
#define _keymapper__hpp__included__

#include <string>
#include <sstream>
#include <stdexcept>
#include <list>
#include <set>
#include <map>
#include <iostream>
#include "misc.hpp"

/**
 * Takes in a raw command and returns the command that should be actually executed given the key polarity.
 *
 * parameter cmd: Raw command.
 * parameter polarity: Polarity (True => Being pressed, False => Being released).
 * returns: The fixed command, "" if no command should be executed.
 * throws std::bad_alloc: Not enough memory.
 */
std::string fixup_command_polarity(std::string cmd, bool polarity) throw(std::bad_alloc);



/**
 * Modifier key.
 *
 * Each object of this class is a modifier key (e.g. left control) or group of modifier keys (e.g. control).
 */
class modifier
{
public:
/**
 * Create a new modifier.
 *
 * parameter name: Name of the modifier.
 * throws std::bad_alloc: Not enough memory.
 */
	modifier(const std::string& name) throw(std::bad_alloc);
/**
 * Create a new linked modifier.
 *
 * If modifiers A and B are both linked to C, then:
 * - It is legal to specify A and/or B as modifier when modifier mask contains C.
 * - If modifier contains C, then both A and B activate it.
 *
 * The usual use for linked modifiers is when there are two closely related keys (e.g. left ctrl and right ctrl)
 * one wants to be able to be referred with single name.
 *
 * parameter name: Name of the modifier.
 * parameter linkgroup: The name of modifier this modifier is linked to (this modifier should be created).
 * throws std::bad_alloc: Not enough memory.
 */
	modifier(const std::string& name, const std::string& linkgroup) throw(std::bad_alloc);
/**
 * Destructor
 */
	~modifier() throw();
/**
 * Look up a modifier.
 *
 * parameter name: The name of the modifier to look up.
 * returns: The looked up modifier.
 * throws std::bad_alloc: Not enough memory.
 * throws std::runtime_error: No such modifier is known.
 */
	static modifier& lookup(const std::string& name) throw(std::bad_alloc, std::runtime_error);
/**
 * Get name of modifier.
 *
 * returns: The name of this modifier.
 * throws: std::bad_alloc: Not enough memory.
 */
	std::string name() const throw(std::bad_alloc);
/**
 * Get name of linked modifier.
 *
 * returns: The name of linked modifier, "" if none.
 * throws: std::bad_alloc: Not enough memory.
 */
	std::string linked_name() const throw(std::bad_alloc);
/**
 * Get set of all modifiers.
 */
	static std::set<std::string> get_set() throw(std::bad_alloc);
private:
	modifier(const modifier&);
	modifier& operator=(const modifier&);
	std::string modname;
};

/**
 * A set of modifier keys.
 */
class modifier_set
{
public:
/**
 * Add a modifier into the set.
 *
 * parameter mod: The modifier to add.
 * parameter really: If true, actually add the key. If false, do nothing.
 * throws std::bad_alloc: Not enough memory.
 */
	void add(const modifier& mod, bool really = true) throw(std::bad_alloc);
/**
 * Remove a modifier from the set.
 *
 * parameter mod: The modifier to remove.
 * parameter really: If true, actually remove the key. If false, do nothing.
 * throws std::bad_alloc: Not enough memory.
 */
	void remove(const modifier& mod, bool really = true) throw(std::bad_alloc);
/**
 * Construct modifier set from comma-separated string.
 *
 * parameter modifiers: The modifiers as string
 * returns: The constructed modifier set.
 * throws std::bad_alloc: Not enough memory.
 * throws std::runtime_error: Illegal modifier or wrong syntax.
 */
	static modifier_set construct(const std::string& modifiers) throw(std::bad_alloc, std::runtime_error);
/**
 * Check modifier against its mask for validity.
 *
 * This method checks that:
 * - for each modifier in set, either that or its linkage group is in mask.
 * - Both modifier and its linkage group isn't in either set or mask.
 *
 * parameter set: The set to check.
 * parameter mask: The mask to check against.
 * returns: True if set is valid, false if not.
 * throws std::bad_alloc: Not enough memory.
 */
	static bool valid(const modifier_set& set, const modifier_set& mask) throw(std::bad_alloc);
/**
 * Check if this modifier set triggers the action.
 *
 * Modifier set triggers another if for each modifier or linkage group in mask:
 * - Modifier appears in both set and trigger.
 * - At least one modifier with this linkage group appears in both set and trigger.
 * - Modifiers with this linkage group do not appear in either set nor trigger.
 *
 */
	static bool triggers(const modifier_set& set, const modifier_set& trigger, const modifier_set& mask)
		throw(std::bad_alloc);
/**
 * Equality check.
 *
 * parameter m: Another set.
 * returns: True if two sets are equal, false if not.
 */
	bool operator==(const modifier_set& m) const throw();

private:
	friend std::ostream& operator<<(std::ostream& os, const modifier_set& m);
	std::set<const modifier*> set;
};

/**
 * Debugging print. Prints textual version of set into stream.
 *
 * parameter os: The stream to print to.
 * parameter m: The modifier set to print.
 * returns: reference to os.
 */
std::ostream&  operator<<(std::ostream& os, const modifier_set& m);

/**
 * Key or key group.
 *
 * Each object of this type is either one key or group of keys.
 */
class keygroup
{
public:
/**
 * Key group type.
 */
	enum type
	{
/**
 * Disabled.
 */
		KT_DISABLED,
/**
 * Singular button.
 */
		KT_KEY,
/**
 * Pressure-sensitive button
 */
		KT_PRESSURE_PM,
		KT_PRESSURE_MP,
		KT_PRESSURE_0P,
		KT_PRESSURE_0M,
		KT_PRESSURE_P0,
		KT_PRESSURE_M0,
/**
 * Axis key pair.
 */
		KT_AXIS_PAIR,
		KT_AXIS_PAIR_INVERSE,
/**
 * Hat.
 */
		KT_HAT,
/**
 * Mouse axis (this is not a real axis!).
 */
		KT_MOUSE
	};
/**
 * Create a new key group.
 *
 * parameter name: Name of the key group.
 * parameter t: Initial type of the key group.
 * throws std::bad_alloc: Not enough memory.
 */
	keygroup(const std::string& name, enum type t) throw(std::bad_alloc);
/**
 * Destructor
 */
	~keygroup() throw();
/**
 * Lookup key group by name.
 *
 * Parameter name: The key group name.
 * Returns: The looked up key group, or NULL if not found.
 */
	static keygroup* lookup_by_name(const std::string& name) throw();
/**
 * Get the set of axes.
 *
 * Returns: The axis set (all axes).
 * Throws std::bad_alloc: Not enough memory.
 */
	static std::set<std::string> get_axis_set() throw(std::bad_alloc);
/**
 * Change type of key group.
 *
 * parameter t: New type for the key group.
 */
	void change_type(enum type t) throw();
/**
 * Change calibration (Axis pairs and pressure buttons only).
 *
 * parameter left: The control value at extreme negative position.
 * parameter center: The control value at center position.
 * parameter right: The control value at extreme positive position.
 * parameter tolerance: How wide is the neutral zone (must be larger than 0 and smaller than 1).
 */
	void change_calibration(short left, short center, short right, double tolerance);
/**
 * Change state of this key group.
 *
 * For KT_KEY, value is zero/nonzero.
 * For KT_PRESSURE_* and KT_AXIS_PAIR*, value is -32768...32767.
 * For KT_HAT, 1 is up, 2 is right, 4 is down, 8 is left (may be ORed).
 * For KT_MOUSE, value is -32768...32767.
 *
 * parameter pos: New position.
 * parameter modifiers: The modifier set that was pressed during the change.
 */
	void set_position(short pos, const modifier_set& modifiers) throw();
/**
 * Look up individual key by name.
 *
 * parameter name: The name of the key to look up.
 * returns: First element is pointer to key group, second is key index within the group.
 * throws std::bad_alloc: Not enough memory.
 * throws std::runtime_error: No such key known.
 */
	static std::pair<keygroup*, unsigned> lookup(const std::string& name) throw(std::bad_alloc,
		std::runtime_error);
/**
 * Look up key group name.
 *
 * returns: The name of the key group.
 * throws std::bad_alloc: Not enough memory.
 */
	std::string name() throw(std::bad_alloc);
/**
 * Get set of all keys (including subkeys).
 */
	static std::set<std::string> get_keys() throw(std::bad_alloc);

/**
 * Key group parameters.
 */
	struct parameters
	{
/**
 * Type
 */
		enum type ktype;
/**
 * Last known raw value.
 */
		short last_rawval;
/**
 * Calibration left.
 */
		short cal_left;
/**
 * Calibration center.
 */
		short cal_center;
/**
 * Calibration right.
 */
		short cal_right;
/**
 * Calibration tolerance.
 */
		double cal_tolerance;
	};
/**
 * Get parameters.
 */
	struct parameters get_parameters();
/**
 * Get all key parameters.
 */
	static std::map<std::string, struct parameters> get_all_parameters();
/**
 * Set callback requests on/off
 */
	void request_hook_callback(bool state);
/**
 * Get status value.
 */
	signed get_value();
private:
	signed state;
	enum type ktype;
	short last_rawval;
	short cal_left;
	short cal_center;
	short cal_right;
	double cal_tolerance;
	double compensate(short value);
	double compensate2(double value);
	void run_listeners(const modifier_set& modifiers, unsigned subkey, bool polarity, bool really, double x);
	std::string keyname;
	bool requests_hook;
};

/**
 * This class handles internals of mapping events from keyboard buttons and pseudo-buttons.
 */
class keymapper
{
public:
/**
 * Binds a key, erroring out if binding would conflict with existing one.
 *
 * parameter mod: Modifier set to require to be pressed.
 * parameter modmask: Modifier set to take into account.
 * parameter keyname: Key to bind the action to.
 * parameter command: The command to bind.
 * throws std::bad_alloc: Not enough memory.
 * throws std::runtime_error: The binding would conflict with existing one or invalid modifier/key.
 */
	static void bind(std::string mod, std::string modmask, std::string keyname, std::string command)
		throw(std::bad_alloc, std::runtime_error);
/**
 * Unbinds a key, erroring out if binding does not exist..
 *
 * parameter mod: Modifier set to require to be pressed.
 * parameter modmask: Modifier set to take into account.
 * parameter keyname: Key to bind the action to.
 * throws std::bad_alloc: Not enough memory.
 * throws std::runtime_error: The binding does not exist.
 */
	static void unbind(std::string mod, std::string modmask, std::string keyname) throw(std::bad_alloc,
		std::runtime_error);
/**
 * Dump list of bindigns as message to console.
 *
 * throws std::bad_alloc: Not enough memory.
 */
	static void dumpbindings() throw(std::bad_alloc);
/**
 * Get keys bound.
 */
	static std::set<std::string> get_bindings() throw(std::bad_alloc);
/**
 * Get command for key.
 */
	static std::string get_command_for(const std::string& keyspec) throw(std::bad_alloc);
/**
 * Bind command for key.
 */
	static void bind_for(const std::string& keyspec, const std::string& cmd) throw(std::bad_alloc,
		std::runtime_error);
};

#endif
