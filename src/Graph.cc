
#include <complex>
#include <iostream>
#include <cmath>
#include <memory>
#include <type_traits>
#include <string>
#include <fstream>
#include <map>

#include "Parser.hh"
#include "Graph.hh"
#include "Objects.hh"

namespace Volsung {

Number::operator float&()
{
    return real_part;
}

Number::operator float() const
{
    return real_part;
}

Number::operator Text() const
{
    if (!std::isfinite(real_part)) return std::to_string(real_part);
    std::string ret = "";
    bool const real = std::abs(real_part) >= 0.001;
    bool const imag = std::abs(imag_part) >= 0.001;

    if (real) {
        ret += std::to_string(real_part);
        ret.erase(ret.end() - 3, ret.end());
        if (imag) ret += " + ";
    }

    if (imag) {
        ret += std::to_string(imag_part);
        ret.erase(ret.end() - 3, ret.end());
        ret += "i";
    }
    if (ret.empty()) ret = "0";

    return (Text) ret;
}

bool Number::is_complex() const
{
    return (bool) imag_part;
}

float Number::magnitude() const
{
    float const value = std::sqrt(real_part * real_part + imag_part * imag_part);
    return value;
}

float Number::angle() const
{
    return std::atan2(imag_part, real_part);
}

float& Number::real()
{
    return real_part;
}

float& Number::imag()
{
    return imag_part;
}

Number Number::negated() const
{
    return Number(-real_part, -imag_part);
}

Number::Number(float initial_value) : real_part(initial_value) {}

Number::Number(float initial_real_part, float initial_imag_part)
    : real_part(initial_real_part),
      imag_part(initial_imag_part) {}


#define DEFINE_ARITHMETIC_OPERATION_ON_NUMBER(op)                               \
TypedValue Number::op(const TypedValue& other)                                  \
{                                                                               \
    switch (other.get_type()) {                                                 \
        case (Type::number): return op##_num(other.get_value<Number>());        \
        case (Type::sequence): {                                                \
            Sequence seq = other.get_value<Sequence>();                         \
            for (auto& element: seq) element = op##_num(element);               \
            return seq;                                                         \
        }                                                                       \
        default: error("Attempted to perform arithmetic on non-numeric value"); \
    }                                                                           \
    return TypedValue(0);                                                       \
}                                                                               \

DEFINE_ARITHMETIC_OPERATION_ON_NUMBER(add)
DEFINE_ARITHMETIC_OPERATION_ON_NUMBER(subtract)
DEFINE_ARITHMETIC_OPERATION_ON_NUMBER(multiply)
DEFINE_ARITHMETIC_OPERATION_ON_NUMBER(divide)
DEFINE_ARITHMETIC_OPERATION_ON_NUMBER(exponentiate)
#undef DEFINE_ARITHMETIC_OPERATION_ON_NUMBER

#define DEFINE_ARITHMETIC_OPERATION_ON_SEQUENCE(op)                             \
TypedValue Sequence::op(const TypedValue& other)                                \
{                                                                               \
    switch (other.get_type()) {                                                 \
        case (Type::number): {                                                  \
            const Number value = other.get_value<Number>();                     \
            for (auto& element: *this) element = element.op##_num(value);       \
            return *this;                                                       \
        }                                                                       \
        case (Type::sequence): {                                                \
            Sequence seq = other.get_value<Sequence>();                         \
            Volsung::assert(size() == seq.size(), "Attempted to perform arithmetic on sequences of inequal length");       \
            for (size_t n = 0; n < size(); n++)                                 \
                seq[n] = data[n].op##_num(seq[n]);                              \
            return seq;                                                         \
        }                                                                       \
        default: error("Attempted to perform arithmetic on non-numeric value"); \
    }                                                                           \
    return TypedValue(0);                                                       \
}                                                                               \

DEFINE_ARITHMETIC_OPERATION_ON_SEQUENCE(add)
DEFINE_ARITHMETIC_OPERATION_ON_SEQUENCE(subtract)
DEFINE_ARITHMETIC_OPERATION_ON_SEQUENCE(multiply)
DEFINE_ARITHMETIC_OPERATION_ON_SEQUENCE(divide)
DEFINE_ARITHMETIC_OPERATION_ON_SEQUENCE(exponentiate)
#undef DEFINE_ARITHMETIC_OPERATION_ON_SEQUENCE


Number Number::add_num(const Number& other) const
{
    return Number(real_part + other.real_part,
           imag_part + other.imag_part);
}

Number Number::subtract_num(const Number& other) const
{
    return Number(real_part - other.real_part,
           imag_part - other.imag_part);
}

Number Number::multiply_num(const Number& other) const
{
    const float new_real_part = real_part * other.real_part - imag_part * other.imag_part;
    const float new_imag_part = imag_part * other.real_part + real_part * other.imag_part;
    return Number(new_real_part, new_imag_part);
}

Number Number::divide_num(const Number& other) const
{
    if (is_complex()) {
        if (!other.is_complex()) return Number(real_part / other.real_part, imag_part / other.real_part);

        const Number conjugate = Number(real_part, -imag_part);
        const Number denominator = other.multiply_num(conjugate);
        const Number inter = multiply_num(conjugate);
        return Number(inter.real_part / denominator.real_part, inter.imag_part / denominator.real_part);
    }
    return (real_part / other.real_part);
}

Number Number::exponentiate_num(const Number& other) const
{
    float i = imag_part;
    if (i == -0.f) i = 0.f;
    const auto complex = std::pow(std::complex(real_part, i), std::complex(other.real_part, other.imag_part));
    return Number(complex.real(), complex.imag());
}

size_t Sequence::size() const
{
    return data.size();
}

Sequence::operator Text() const
{
    std::string string = "{ ";

    if (data.size()) string += (std::string) (Text) data[0];
    for (size_t n = 1; n < size(); n++) string += ", " + (std::string) (Text) data[n];
    string += " }";

    return Text(string);
}

void Sequence::add_element(const Number value)
{
    data.push_back(value);
}

void Sequence::perform_range_check(const long long n) const
{
    if (n >= long(size()) || n < -long(size()))
        error("Sequence index out of range. Index is: " + std::to_string(n) + ", length is: " + std::to_string(size()));
}

Number* Sequence::get_data_pointer()
{
    return data.data();
}

Number& Sequence::operator[](long long n)
{
    if (n < 0) n += size();
    perform_range_check(n);
    return data[n];
}

const Number& Sequence::operator[](long long n) const
{
    if (n < 0) n += size();
    perform_range_check(n);
    return data.at(n);
}

// Sequence::Sequence() : identifier(sequence_table.size())
// {
//    
// }

Sequence::Sequence(const std::vector<float>& _data)
{
    data.resize(_data.size());
    for (size_t n = 0; n < _data.size(); n++) data[n] = _data[n];
}

Type TypedValue::get_type() const
{
    if (is_type<Number>()) return Type::number;
    if (is_type<Sequence>()) return Type::sequence;
    if (is_type<Procedure>()) return Type::procedure;
    return Type::text;
}

std::string TypedValue::as_string() const
{
    switch(get_type()) {
        case(Type::number): return (Text) get_value<Number>();
        case(Type::sequence): return (Text) get_value<Sequence>();
        case(Type::text): return get_value<Text>();
        case(Type::procedure): return "PROCEDURE";
    }
    return "";
}


void TypedValue::operator+=(const TypedValue& other)
{
    switch(get_type()) {
        case(Type::number): *this = get_value<Number>().add(other); break;
        case(Type::sequence): *this = get_value<Sequence>().add(other); break;
        default: error("Attempted to perform arithmetic on non-numeric value");
    }
}

void TypedValue::operator-=(const TypedValue& other)
{
    switch(get_type()) {
        case(Type::number): *this = get_value<Number>().subtract(other); break;
        case(Type::sequence): *this = get_value<Sequence>().subtract(other); break;
        default: error("Attempted to perform arithmetic on non-numeric value");
    }
}

void TypedValue::operator*=(const TypedValue& other)
{
    switch(get_type()) {
        case(Type::number): *this = get_value<Number>().multiply(other); break;
        case(Type::sequence): *this = get_value<Sequence>().multiply(other); break;
        default: error("Attempted to perform arithmetic on non-numeric value");
    }
}

void TypedValue::operator/=(const TypedValue& other)
{
    switch(get_type()) {
        case(Type::number): *this = get_value<Number>().divide(other); break;
        case(Type::sequence): *this = get_value<Sequence>().divide(other); break;
        default: error("Attempted to perform arithmetic on non-numeric value");
    }
}

void TypedValue::operator^=(const TypedValue& other)
{
    switch(get_type()) {
        case(Type::number): *this = get_value<Number>().exponentiate(other); break;
        case(Type::sequence): *this = get_value<Sequence>().exponentiate(other); break;
        default: error("Attempted to perform arithmetic on non-numeric value");
    }
}

TypedValue& TypedValue::operator-()
{
    switch(get_type()) {
        case(Type::number): *this = this->get_value<Number>().negated(); break;
        case(Type::sequence): for (auto& value: this->get_value<Sequence>()) value = -value; break;
        default: error("Attempted to perform arithmetic on non-numeric value");
    }
    return *this;
}


TypedValue Procedure::operator()(const ArgumentList& args, Program* program) const
{
    Volsung::assert((bool) implementation, "Internal error: procedure has no implementation");

    if (can_be_mapped && args.size() && args[0].is_type<Sequence>()) {
        auto sequence = args[0].get_value<Sequence>();
        auto parameters = args;

        for (size_t n = 0; n < sequence.size(); n++) {
            parameters[0] = sequence[n];
            sequence[n] = implementation(parameters, program).get_value<Number>();
        }
        return sequence;
    }
    return implementation(args, program);
}

Procedure::Procedure(Implementation impl, size_t min_args, size_t max_args, bool _can_be_mapped)
    : implementation(impl), min_arguments(min_args), max_arguments(max_args), can_be_mapped(_can_be_mapped)
{ }

#define APPLY_FLOAT_FUNCTION_TO_NUMBER(fun)      \
    Number number = args[0].get_value<Number>(); \
    number.imag() = fun(number.imag());          \
    number.real() = fun(number.real());          \
    return number                                \

const SymbolTable<Procedure> Program::procedures = {
    { "random", Procedure([] (const ArgumentList& arguments, Program*) -> TypedValue {
        float min = 0.f;
        float max = 1.f;

        if (arguments.size() >= 1) max = arguments[0].get_value<Number>();
        if (arguments.size() == 2) {
            min = max;
            max = arguments[1].get_value<Number>();
        }

        std::uniform_real_distribution<float> distribution(min, max);
        static std::default_random_engine generator((uint) std::chrono::system_clock::now().time_since_epoch().count());

        return (Number) distribution(generator);
    }, 0, 2) },

    { "Arg", Procedure([] (const ArgumentList& args, Program*) {
        return args[0].get_value<Number>().angle();
    }, 1, 1, true)},

    { "abs", Procedure([] (const ArgumentList& args, Program*) {
        return args[0].get_value<Number>().magnitude();
    }, 1, 1, true)},

    { "mod", Procedure([] (const ArgumentList& args, Program*) {
        Number lhs = args[0].get_value<Number>();
        Number rhs = args[1].get_value<Number>();
        return (Number) std::fmod((float) lhs, (float) rhs);
    }, 2, 2, true)},

    { "sin", Procedure([] (const ArgumentList& args, Program*) {
        return std::sin(args[0].get_value<Number>());
    }, 1, 1, true)},

    { "cos", Procedure([] (const ArgumentList& args, Program*) {
        return std::cos(args[0].get_value<Number>());
    }, 1, 1, true)},

    { "ceil", Procedure([] (const ArgumentList& args, Program*) {
        APPLY_FLOAT_FUNCTION_TO_NUMBER(std::ceil);
    }, 1, 1, true)},

    { "tanh", Procedure([] (const ArgumentList& args, Program*) {
        APPLY_FLOAT_FUNCTION_TO_NUMBER(std::tanh);
    }, 1, 1, true)},

    { "atan", Procedure([] (const ArgumentList& args, Program*) {
        APPLY_FLOAT_FUNCTION_TO_NUMBER(std::atan);
    }, 1, 1, true)},

    { "floor", Procedure([] (const ArgumentList& args, Program*) {
        APPLY_FLOAT_FUNCTION_TO_NUMBER(std::floor);
    }, 1, 1, true)},

    { "sign", Procedure([] (const ArgumentList& args, Program*) {
        return float(args[0].get_value<Number>()) >= 0.f ? 1 : -1;
    }, 1, 1, true)},

    { "clamp", Procedure([] (const ArgumentList& args, Program*) {
        return std::clamp(args[0].get_value<Number>(), args[1].get_value<Number>(), args[2].get_value<Number>());
    }, 3, 3, true)},

    { "sqrt", Procedure([] (const ArgumentList& args, Program*) {
        return args[0].get_value<Number>().exponentiate_num(0.5f);
    }, 1, 1, true)},

    { "ln", Procedure([] (const ArgumentList& args, Program*) {
        return std::log(args[0].get_value<Number>());
    }, 1, 1, true)},

    { "log", Procedure([] (const ArgumentList& args, Program*) {
        float base = 10.f;
        if (args.size() > 1) base = args[1].get_value<Number>();
        const float value = args[0].get_value<Number>();

        return std::log(value) / std::log(base);
    }, 1, 2, true)},

    { "Re", Procedure([] (const ArgumentList& args, Program*) {
        return (float) args[0].get_value<Number>();
    }, 1, 1, true)},

    { "Im", Procedure([] (const ArgumentList& args, Program*) {
        Number num = args[0].get_value<Number>();
        return num.imag();
    }, 1, 1, true)},

    { "conjugate", Procedure([] (const ArgumentList& args, Program*) {
        Number num = args[0].get_value<Number>();
        return Number(num.real(), -num.imag());
    }, 1, 1, true)},

    { "reverse", Procedure([] (const ArgumentList& args, Program*) {
        const Sequence source = args[0].get_value<Sequence>();
        Sequence reverse;
        for (size_t n = 1; n <= source.size(); n++) {
            reverse.add_element(source[source.size() - n]);
        }
        return reverse;
    }, 1, 1)},

    { "concatenate", Procedure([] (const ArgumentList& args, Program*) -> TypedValue {
        if (args[0].is_type<Sequence>()) {
            const Sequence a = args[0].get_value<Sequence>();
            const Sequence b = args[1].get_value<Sequence>();

            Sequence out;
            for (size_t n = 0; n < a.size() + b.size(); n++) {
                out.add_element(n < a.size() ? a[n] : b[n-a.size()]);
            }

            return out;
        }

        const Text a = args[0].get_value<Text>();
        const Text b = args[1].get_value<Text>();
        return (Text) (std::string) a + (std::string) b;
    }, 2, 2)},

    { "map", Procedure([] (const ArgumentList& args, Program* program) {
        const Procedure proc = args[1].get_value<Procedure>();
        const Sequence source = args[0].get_value<Sequence>();
        Sequence mapped;
        for (size_t n = 0; n < source.size(); n++) {
            mapped.add_element(proc(ArgumentList { source[n], n }, program).get_value<Number>());
        }
        return mapped;
    }, 2, 2)},

    { "sum", Procedure([] (const ArgumentList& args, Program*) {
        Number sum = 0.f;
        Sequence sequence = args[0].get_value<Sequence>();
        for (auto element: sequence) {
            sum += element;
        }
        return sum;
    }, 1, 1)},

    { "average", Procedure([] (const ArgumentList& args, Program*) {
        Number sum = 0.f;
        Sequence sequence = args[0].get_value<Sequence>();
        for (auto element: sequence) {
            sum += element;
        }
        return sum / sequence.size();
    }, 1, 1)},

    { "greatest", Procedure([] (const ArgumentList& args, Program*) {
        Sequence sequence = args[0].get_value<Sequence>();
        Number greatest = sequence[0];
        for (size_t n = 0; n < sequence.size(); n++) {
            if (sequence[n].magnitude() > greatest.magnitude()) {
                greatest = sequence[n];
            }
        }
        return greatest;
    }, 1, 1)},

    { "smallest", Procedure([] (const ArgumentList& args, Program*) {
        Sequence sequence = args[0].get_value<Sequence>();
        Number smallest = sequence[0];
        for (size_t n = 0; n < sequence.size(); n++) {
            if (sequence[n].magnitude() < smallest.magnitude()) {
                smallest = sequence[n];
            }
        }
        return smallest;
    }, 1, 1)},

    { "print", Procedure([] (const ArgumentList& args, Program*) {
        std::string message = "";
        for (const auto& arg : args) message += arg.as_string();
        Volsung::log(message);
        return 0;
    }, 1, -1)},

    { "length_of", Procedure([] (const ArgumentList& args, Program*) {
        return args[0].get_value<Sequence>().size();
    }, 1, 1)},

    { "type_of", Procedure([] (const ArgumentList& args, Program*) {
        return Text(type_name(args[0].get_type()));
    }, 1, 1)},

    { "read_file", Procedure([] (const ArgumentList& args, Program*) {
        const std::string filename = args[0].get_value<Text>();
        std::ifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);
        if (!file) error("Could not read file, not found: '" + filename + "'");

        std::vector<float> out_data;
        if (file.good()) {
            out_data.resize(file.tellg() / sizeof(float));
            file.seekg(0);
            file.read(reinterpret_cast<char*>(out_data.data()), out_data.size() * sizeof(float));
        }
        else error("Could not read file, not found: '" + filename + "'");
        return (Sequence) out_data;
    }, 1, 1)},

    { "write_file", Procedure([](const ArgumentList& args, Program*) {
        const Sequence in_data = args[1].get_value<Sequence>();
        const std::string filename = args[0].get_value<Text>();

        std::ofstream file(filename, std::fstream::out | std::fstream::binary);
        for (size_t n = 0; n < in_data.size(); n++)
            file.write((const char*) &in_data[n], sizeof (float));

        file.close();
        return Number(0);
    }, 2, 2)},

    { "implementation_of", Procedure([] (const ArgumentList& args, Program* program) {
        const std::string object_type = args[0].get_value<Text>();
        if (!program->subgraphs.count(object_type))
            error("'implementation_of(" + object_type + ")': Subgraph implementation not found");

        return (Text) program->subgraphs.at(object_type).first;
    }, 1, 1)},

    { "repeat", Procedure([] (const ArgumentList& args, Program*) {
        Sequence sequence        = args[0].get_value<Sequence>();
        const size_t num_repeats = args[1].get_value<Number>();
        Sequence output;

        for (size_t n = 0; n < num_repeats; n++) {
            for (const auto element: sequence) {
                output.add_element(element);
            }
        }

        return output;
    }, 2, 2)},

    { "count_nodes", Procedure([] (const ArgumentList&, Program* program) {
        Program* current_program = program;
        int num_nodes = 0;

        while (true) {
            num_nodes += current_program->table.size();
            if (current_program->parent) current_program = current_program->parent;
            else break;
        }

        return num_nodes;
    }, 0, 0)},

    { "import_library", Procedure([] (const ArgumentList& args, Program* program) {
        const std::string filename = args[0].get_value<Text>();
        std::ifstream file(get_library_path() + filename);

        if (!file) file.open(get_library_path() + filename + ".vlsng");
        if (!file) file.open(filename);
        if (!file) file.open(filename + ".vlsng");

        if (!file) error("Library not available: '" + filename + "'");

        Parser parser;

        {
            std::stringstream buffer;
            buffer << file.rdbuf();
            parser.source_code = buffer.str();
        }

        if (!parser.parse_program(*program)) error("Library failed to parse. Exiting");
        return Number(0);
    }, 1, 1)},

    { "run_subgraph", Procedure([] (const ArgumentList& args, Program* program) {
        Graph meta_graph;
        Parser parser;

        const SubgraphRepresentation& subgraph_rep = program->subgraphs[args[0].get_value<Text>()];
        const float sample_count = args[1].get_value<Number>();
        parser.source_code = subgraph_rep.first;

        meta_graph.configure_io(subgraph_rep.second[0], subgraph_rep.second[1]);
        meta_graph.reset();
        parser.parse_program(meta_graph);

        Sequence ret;
        for (size_t n = 0; n < sample_count / AudioBuffer::blocksize; n++) {
            auto data = meta_graph.run();
            for (auto const value: data[0]) {
                if (ret.size() >= sample_count) break;
                ret.add_element(value);
            }
        }
        return ret;
    }, 2, 2)},

    { "DFT", Procedure([] (const ArgumentList& args, Program*) {
        Sequence data = args[0].get_value<Sequence>();
        Sequence ret;

        for (size_t n = 0; n < data.size(); n++)
        {
            Number complex;
            for (size_t s = 0; s < data.size(); s++)
            {
                float real = data[s] * std::sin(TAU * s * n / data.size());
                float imag = data[s] * std::cos(TAU * s * n / data.size()) * -1.f;
    
                complex = complex.add_num(Number(real, imag));
            }
            complex = complex.divide_num(Number(data.size(), 0));
            ret.add_element(complex);
        }

        return ret;
    }, 1, 1)},

    { "FFT", Procedure([] (const ArgumentList& args, Program*) {
        Sequence data = args[0].get_value<Sequence>();

        std::function<void(Number*, size_t)> fft = [&fft] (Number* input_data, size_t N) {
            if (N < 2) return;
            // assert((size_t) std::floor(std::log2(N)) == N, "FFT size must be a power of 2");

            size_t M = N / 2;

            auto temp = (Number*) std::malloc(sizeof( Number ) * M);
            for (size_t n = 0; n < M; n++) temp[n] = input_data[n * 2 + 1];
            for (size_t n = 0; n < M; n++) input_data[n] = input_data[n * 2];
            for (size_t n = 0; n < M; n++) input_data[n + M] = temp[n];
            free(temp);

            fft(input_data, M);
            fft(input_data + M, M);

            for (size_t k = 0; k < M; k++)
            {
                Number even = input_data[k];
                Number odd  = input_data[k + M];

                const float theta = -TAU * float(k) / N;
                Number w   = Number(std::cos(theta), std::sin(theta)).multiply_num(odd);
                input_data[k]     = even.add_num(w);
                input_data[k + M] = even.subtract_num(w);
            }
        };

        fft(data.get_data_pointer(), data.size());
        for (auto& value: data) value = value.divide_num(data.size());
        return data;
    }, 1, 1)},
};

#undef APPLY_FLOAT_FUNCTION_TO_NUMBER

void Program::create_user_object(const std::string& name, const uint num_inputs, const uint num_outputs, std::any user_data, const AudioProcessingCallback callback)
{
    if (table.count(name) != 0) error("Symbol '" + name + "' is already used");
    table[name] = std::make_unique<UserObject, ArgumentList, const AudioProcessingCallback&, std::any&>({ TypedValue(num_inputs), TypedValue(num_outputs) }, callback, user_data);
}

void Program::check_io_and_connect_objects(const std::string& output_object, const uint output_index,
                                           const std::string& input_object , const uint input_index)
{
    if (table[output_object]->outputs.size() <= output_index) {
        error("Index out of range on output object '" + output_object + "'. Index is: " + std::to_string(output_index));
    }

    if (table[input_object]->inputs.size() <= input_index)
        error("Index out of range on input object '" + input_object + "'. Index is: " + std::to_string(input_index));

    table[output_object]->outputs[output_index].connect(table[input_object]->inputs.at(input_index));
}

void Program::expect_to_be_group(const std::string& name) const
{
    if (table.count(name)) error(name + " is an object, not a group");
    else if (!group_sizes.count(name)) error("Group " + name + " has not been declared");
}

void Program::expect_to_be_object(const std::string& name) const
{
    if (group_sizes.count(name)) error(name + " is a group, not an object");
    else if (!table.count(name)) error("Object " + name + " has not been declared");
}

void Program::connect_objects(const std::string& output_object, const uint out,
                              const std::string& input_object, const uint in, const ConnectionType type)
{
    if (type == ConnectionType::one_to_one) {
        expect_to_be_object(output_object);
        expect_to_be_object(input_object);
        check_io_and_connect_objects(output_object, out, input_object, in);
    }

    else if (type == ConnectionType::many_to_one) {
        expect_to_be_group(output_object);
        expect_to_be_object(input_object);
        for (size_t n = 0; n < group_sizes[output_object]; n++)
            check_io_and_connect_objects("__grp_" + output_object + std::to_string(n), out, input_object, in);
    }

    else if (type == ConnectionType::one_to_many) {
        expect_to_be_object(output_object);
        expect_to_be_group(input_object);
        for (size_t n = 0; n < group_sizes[input_object]; n++)
            check_io_and_connect_objects(output_object, out, "__grp_" + input_object + std::to_string(n), in);
    }

    else if (type == ConnectionType::series) {
        expect_to_be_group(input_object);
        for (size_t n = 0; n < group_sizes[input_object] - 1; n++)
            check_io_and_connect_objects("__grp_" + input_object + std::to_string(n), 0, "__grp_" + input_object + std::to_string(n + 1), in);
    }

    else if (type == ConnectionType::biclique) {
        expect_to_be_group(output_object);
        expect_to_be_group(input_object);
        for (size_t na = 0; na < group_sizes[output_object]; na++) {
            for (size_t nb = 0; nb < group_sizes[input_object]; nb++) {
                check_io_and_connect_objects("__grp_" + output_object + std::to_string(na), out,
                                "__grp_" + input_object + std::to_string(nb), in);
            }
        }
    }

    else if (type == ConnectionType::many_to_many) {
        expect_to_be_group(output_object);
        expect_to_be_group(input_object);
        if (group_sizes[output_object] != group_sizes[input_object]) error("Group sizes to be connected in parallel are not identical");
        for (size_t n = 0; n < group_sizes[output_object]; n++)
            check_io_and_connect_objects("__grp_" + output_object + std::to_string(n), out,
                            "__grp_" + input_object + std::to_string(n), in);
    }
}

bool Program::object_exists(const std::string& name) const
{
    if (table.count(name) || group_sizes.count(name)) return true;
    return false;
}

void Program::simulate()
{
    for (const auto& entry : table) {
        entry.second->implement();
    }
}

MultichannelBuffer Program::run()
{
    return run( { AudioBuffer::zero } );
}

MultichannelBuffer Program::run(const MultichannelBuffer input_buffer)
{
    if (inputs) {
        AudioInputObject* object = get_audio_object_raw_pointer<AudioInputObject>("input");
        object->data = input_buffer;
    }

    simulate();
    MultichannelBuffer output_buffer(outputs);

    if (outputs) {
        AudioOutputObject* object = get_audio_object_raw_pointer<AudioOutputObject>("output");
        output_buffer = object->data;
        object->data.clear();
    }

    return output_buffer;
}

void Program::finish()
{
    for (auto const& entry : table)
        entry.second->finish();
}

void Program::reset()
{
    table.clear();
    symbol_table.clear();
    group_sizes.clear();
    subgraphs.clear();

    if (inputs) create_object<AudioInputObject>("input", { inputs });
    if (outputs) create_object<AudioOutputObject>("output", { outputs });
}

void Program::add_directive(const std::string& name, const DirectiveFunctor function)
{
    if (!custom_directives.count(name))
        custom_directives[name] = function;
}

void Program::invoke_directive(const std::string& name, const ArgumentList& arguments)
{
    if (!custom_directives.count(name)) error("Unknown directive");

    custom_directives.at(name)(arguments, this);
}

void Program::configure_io(const uint i, const uint o)
{
    inputs = i;
    outputs = o;
    out.resize(outputs);
}

void Program::add_symbol(const std::string& identifier, const TypedValue& value)
{
    if (symbol_exists(identifier)) error("Identifier '" + identifier + "' is already in use");
    symbol_table[identifier] = value;
}

void Program::remove_symbol(const std::string& identifier)
{
    if (symbol_exists(identifier)) symbol_table.erase(identifier);
}

bool Program::symbol_exists(const std::string& identifier) const
{
    return symbol_table.count(identifier) == 1;
}

TypedValue Program::get_symbol_value(const std::string& identifier) const
{

    return symbol_table.at(identifier);
}

const SubgraphRepresentation Program::find_subgraph_recursively(std::string name)
{
    if (subgraphs.count(name)) return subgraphs[name];
    if (!parent) error("Object type does not exist " + name);
    return parent->find_subgraph_recursively(name);
}

}
