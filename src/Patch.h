
#pragma once

#include <memory>
#include <istream>
#include <type_traits>
#include <sstream>
#include <any>

#include "StringFormat.h"
#include "Yggdrasil.h"

namespace Yggdrasil {

using st_type = std::unordered_map<std::string, std::unique_ptr<AudioObject>>;

class Program;
using directive_functor = std::function<void(std::vector<std::string>, Program*)>;
using callback_functor = std::function<void(buf&, buf&, std::any)>;

class Program
{
	st_type table;

	uint lines_parsed = 0;
	static inline std::map<std::string, directive_functor> custom_directives;
    Program* parent = nullptr;

	uint inputs = 0;
	uint outputs = 0;
public:

	template<class>
	bool create_object(std::string, std::string = "");

	template<class T>
	T* get_audio_object_raw_pointer(std::string);

	void connect_objects(std::unique_ptr<AudioObject>&, uint,
	                     std::unique_ptr<AudioObject>&, uint);

	static void connect_objects(Program&, std::string, uint, std::string, uint);

	static void add_directive(std::string, directive_functor);
	void create_user_object(std::string, uint, uint, std::any, callback_functor);
											  
	void make_graph(std::istream&&);
	void configure_io(uint, uint);
	
	void run();
	float run(float);
	std::vector<float> run(std::vector<float>);
	void finish();
	void reset();
	
	auto begin() { return std::begin(table); }
	auto end() { return std::end(table); }
};

using Graph = Program;


template<class T>
T* Program::get_audio_object_raw_pointer(std::string name)
{
	static_assert(std::is_base_of<AudioObject, T>::value, "Type must be audio object");
	return static_cast<T*>(table[name].get());
}

}
