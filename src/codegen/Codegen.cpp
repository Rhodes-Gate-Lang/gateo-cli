#include "codegen/Codegen.hpp"

#include "RuntimeEmbed.hpp"
#include "codegen/CodegenConfig.hpp"
#include "gateo/v2/view.hpp"

#include <CLI/CLI.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace gate_cli::codegen {

namespace {

// ── CLI option maps ───────────────────────────────────────────────────────────
//
// Same aliases as `gate sim --output-format`; keep in sync when extending.

const std::map<std::string, gate_cli::NumberFormat> k_number_format_map{
    {"binary",  gate_cli::NumberFormat::Binary},
    {"bin",     gate_cli::NumberFormat::Binary},
    {"dec",     gate_cli::NumberFormat::Dec},
    {"decimal", gate_cli::NumberFormat::Dec},
    {"int",     gate_cli::NumberFormat::Dec},
    {"hex",     gate_cli::NumberFormat::Hex},
};

const char* format_label(gate_cli::NumberFormat f) {
    switch (f) {
        case gate_cli::NumberFormat::Binary: return "binary";
        case gate_cli::NumberFormat::Hex:    return "hex";
        case gate_cli::NumberFormat::Dec:    return "decimal";
        default:                             return "unknown";
    }
}

// ── Emitter helpers ───────────────────────────────────────────────────────────

// Returns the GATE_FMT_* macro name for a NumberFormat, to be emitted into the
// generated C file as the value of GATE_DEFAULT_OUTPUT_FORMAT.
const char* format_to_rt_macro(gate_cli::NumberFormat fmt) {
    switch (fmt) {
        case gate_cli::NumberFormat::Dec:    return "GATE_FMT_DEC";
        case gate_cli::NumberFormat::Binary: return "GATE_FMT_BIN";
        default:                             return "GATE_FMT_HEX";
    }
}

// Computes the bitmask for a node of the given width.
// Matches the mask_width() logic in Evaluation.cpp and gate_rt.c.
std::uint64_t compute_mask(std::uint32_t width) {
    if (width == 0)   return 0;
    if (width >= 64)  return ~std::uint64_t{0};
    return (std::uint64_t{1} << width) - 1;
}

// Formats a uint64 as a lowercase hex C literal (e.g. 0xffULL).
std::string hex64(std::uint64_t v) {
    std::ostringstream ss;
    ss << "0x" << std::hex << v << "ULL";
    return ss.str();
}

// Returns a human-readable type name for inline comments.
const char* gate_type_label(gateo::v2::view::GateType t) {
    using gateo::v2::view::GateType;
    switch (t) {
        case GateType::Input:    return "Input";
        case GateType::Output:   return "Output";
        case GateType::And:      return "And";
        case GateType::Or:       return "Or";
        case GateType::Xor:      return "Xor";
        case GateType::Not:      return "Not";
        case GateType::Literal:  return "Literal";
        default:                 return "Unknown";
    }
}

// Builds the trailing comment for a node variable declaration:
//   "TypeName [width]"  or  "TypeName \"name\" [width]"
std::string node_comment(const gateo::v2::view::Node& n) {
    std::string s = gate_type_label(n.type);
    if (n.name.has_value() && !n.name->empty()) {
        s += " \"" + *n.name + "\"";
    }
    s += " [" + std::to_string(n.width) + "]";
    return s;
}

// ── Design emitter ────────────────────────────────────────────────────────────

// Writes a complete <stem>_design.c to `out`.
//
// The generated file:
//   1. Defines the GATE_INPUT_* / GATE_OUTPUT_* metadata tables used by the
//      runtime to parse arguments and print results.
//   2. Defines design_eval(), which translates the topologically-sorted node
//      list into a straight-line sequence of uint64_t locals — one per node —
//      then stores root-output values into the outputs[] array.
//
// Width masking strategy
// ──────────────────────
// Every node's value is masked to its declared width at write time (i.e. when
// the local variable is assigned).  This is equivalent to what value_of() does
// in Evaluation.cpp (masks at read time).  Not nodes in particular require the
// mask to prevent the bitwise complement from setting bits above the width.
void emit_design_c(
    const gateo::v2::view::GateObject& obj,
    gate_cli::NumberFormat             default_fmt,
    std::string_view                   stem,
    std::ostream&                      out)
{
    using gateo::v2::view::GateType;
    const auto& nodes = obj.nodes;

    // ── Pass 1: identify root inputs / outputs and assign each a slot ────────
    //
    // "Root" means parent == 0 (the top-level component).  Non-root Input/Output
    // nodes are instance ports inside sub-components; they are handled in the
    // eval loop but do not appear in the CLI argument tables.

    std::vector<std::uint32_t>             root_inputs;   // node indices in discovery order
    std::vector<std::uint32_t>             root_outputs;  // node indices in discovery order
    std::unordered_map<std::uint32_t, int> input_slot;    // node_idx → inputs[] slot
    std::unordered_map<std::uint32_t, int> output_slot;   // node_idx → outputs[] slot

    for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(nodes.size()); ++i) {
        const auto& n = nodes[i];
        if (n.type == GateType::Input && n.parent == 0) {
            input_slot[i] = static_cast<int>(root_inputs.size());
            root_inputs.push_back(i);
        } else if (n.type == GateType::Output && n.parent == 0) {
            output_slot[i] = static_cast<int>(root_outputs.size());
            root_outputs.push_back(i);
        }
    }

    const int n_in  = static_cast<int>(root_inputs.size());
    const int n_out = static_cast<int>(root_outputs.size());

    // ── File header ───────────────────────────────────────────────────────────
    out << "/* " << stem << "_design.c — generated by `gate codegen`. Do not edit. */\n\n"
        << "#include \"gate_rt.h\"\n"
        << "#include <stddef.h>\n"   // NULL
        << "#include <stdint.h>\n\n";

    // ── Default output format ─────────────────────────────────────────────────
    out << "/* Output format baked in by gate codegen --output-format. */\n"
        << "const int GATE_DEFAULT_OUTPUT_FORMAT = "
        << format_to_rt_macro(default_fmt) << ";\n\n";

    // ── Input port metadata ───────────────────────────────────────────────────
    out << "/* ── Input ports (" << n_in << ") ───────────────────────────────── */\n";
    if (root_inputs.empty()) {
        // C99 requires at least one initialiser; use a placeholder that the
        // runtime never accesses (guarded by GATE_INPUT_COUNT == 0).
        out << "const char *const GATE_INPUT_NAMES[]  = { NULL };\n"
            << "const uint32_t    GATE_INPUT_WIDTHS[] = { 0u };  /* unused */\n";
    } else {
        out << "const char *const GATE_INPUT_NAMES[] = {";
        for (std::uint32_t idx : root_inputs) {
            out << " \"" << nodes[idx].name.value_or("?") << "\",";
        }
        out << " NULL };\n";

        out << "const uint32_t    GATE_INPUT_WIDTHS[] = {";
        for (std::uint32_t idx : root_inputs) {
            out << " " << nodes[idx].width << "u,";
        }
        out << " };\n";
    }
    out << "const int GATE_INPUT_COUNT = " << n_in << ";\n\n";

    // ── Output port metadata ──────────────────────────────────────────────────
    out << "/* ── Output ports (" << n_out << ") ──────────────────────────────── */\n";
    if (root_outputs.empty()) {
        out << "const char *const GATE_OUTPUT_NAMES[]  = { NULL };\n"
            << "const uint32_t    GATE_OUTPUT_WIDTHS[] = { 0u };  /* unused */\n";
    } else {
        out << "const char *const GATE_OUTPUT_NAMES[] = {";
        for (std::uint32_t idx : root_outputs) {
            out << " \"" << nodes[idx].name.value_or("?") << "\",";
        }
        out << " NULL };\n";

        out << "const uint32_t    GATE_OUTPUT_WIDTHS[] = {";
        for (std::uint32_t idx : root_outputs) {
            out << " " << nodes[idx].width << "u,";
        }
        out << " };\n";
    }
    out << "const int GATE_OUTPUT_COUNT = " << n_out << ";\n\n";

    // ── design_eval ───────────────────────────────────────────────────────────
    out << "/* ── Evaluation ──────────────────────────────────────────────────── */\n"
        << "void design_eval(const uint64_t *inputs, uint64_t *outputs) {\n";

    if (nodes.empty()) {
        out << "    (void)inputs; (void)outputs;\n";
    }

    // Walk the topologically-sorted node list.  Each node becomes one uint64_t
    // local; earlier nodes are always in scope when later nodes reference them.
    for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(nodes.size()); ++i) {
        const auto&       n    = nodes[i];
        const std::string var  = "n" + std::to_string(i);
        const std::uint64_t mask = compute_mask(n.width);

        out << "    uint64_t " << var << " = ";

        switch (n.type) {

            case GateType::Input: {
                if (n.parent == 0) {
                    // Root input: runtime has already parsed and width-masked
                    // this value; no additional masking is needed.
                    out << "inputs[" << input_slot.at(i) << "]";
                } else {
                    // Instance input port: forwards the value of the driving
                    // net in the parent scope (always at a lower index).
                    out << "n" << n.inputs.at(0);
                }
                break;
            }

            case GateType::Literal: {
                const std::uint64_t val = n.literal_value.value_or(0) & mask;
                out << hex64(val);
                break;
            }

            case GateType::And: {
                if (n.inputs.empty()) {
                    // Degenerate: AND identity is all-ones.
                    out << hex64(mask);
                } else {
                    out << "(";
                    for (std::size_t j = 0; j < n.inputs.size(); ++j) {
                        if (j > 0) out << " & ";
                        out << "n" << n.inputs[j];
                    }
                    out << ") & " << hex64(mask);
                }
                break;
            }

            case GateType::Or: {
                if (n.inputs.empty()) {
                    // Degenerate: OR identity is zero.
                    out << "0x0ULL";
                } else {
                    out << "(";
                    for (std::size_t j = 0; j < n.inputs.size(); ++j) {
                        if (j > 0) out << " | ";
                        out << "n" << n.inputs[j];
                    }
                    out << ") & " << hex64(mask);
                }
                break;
            }

            case GateType::Xor: {
                if (n.inputs.empty()) {
                    // Degenerate: XOR identity is zero.
                    out << "0x0ULL";
                } else {
                    out << "(";
                    for (std::size_t j = 0; j < n.inputs.size(); ++j) {
                        if (j > 0) out << " ^ ";
                        out << "n" << n.inputs[j];
                    }
                    out << ") & " << hex64(mask);
                }
                break;
            }

            case GateType::Not: {
                // The mask is critical here: bitwise NOT on a uint64 sets all
                // bits above the node's width, so we must mask them back off.
                out << "(~n" << n.inputs.at(0) << ") & " << hex64(mask);
                break;
            }

            case GateType::Output: {
                // Output nodes simply forward their single driver's value.
                out << "n" << n.inputs.at(0);
                break;
            }

            default: {
                out << "0x0ULL  /* unrecognised GateType */";
                break;
            }
        }

        // Close the declaration with a semicolon and a human-readable comment.
        out << ";  /* " << node_comment(n) << " */\n";

        // Root output nodes: write the value into outputs[] immediately after
        // the declaration so the two lines stay visually adjacent.
        if (n.type == GateType::Output && n.parent == 0) {
            out << "    outputs[" << output_slot.at(i) << "] = " << var << ";\n";
        }
    }

    out << "}\n";
}

// ── File I/O helpers ──────────────────────────────────────────────────────────

void write_text_file(const std::filesystem::path& path, std::string_view content) {
    std::ofstream f(path);
    if (!f) {
        throw std::runtime_error("cannot open for writing: " + path.string());
    }
    f << content;
    if (!f) {
        throw std::runtime_error("write failed: " + path.string());
    }
}

// ── Compilation ───────────────────────────────────────────────────────────────

// Writes the stable runtime files and invokes `cc` to produce a native binary.
//
// Output layout inside `output_dir`:
//   gate_rt.h          — runtime ABI header (written from embedded string)
//   gate_rt.c          — runtime implementation (written from embedded string)
//   <stem>_design.c    — already written by the caller
//   <stem>             — compiled binary (the final artefact)
void compile(
    const std::filesystem::path& output_dir,
    const std::string&           stem,
    std::ostream&                log)
{
    using detail::k_runtime_header;
    using detail::k_runtime_source;

    write_text_file(output_dir / "gate_rt.h", k_runtime_header);
    write_text_file(output_dir / "gate_rt.c", k_runtime_source);

    const std::string rt_c     = (output_dir / "gate_rt.c").string();
    const std::string design_c = (output_dir / (stem + "_design.c")).string();
    const std::string binary   = (output_dir / stem).string();

    // Double-quote each path to handle spaces; the shell expansion is safe as
    // long as paths themselves contain no embedded double-quotes (standard).
    const std::string cmd =
        "cc -std=c11 -Wall"
        " \"" + rt_c     + "\""
        " \"" + design_c + "\""
        " -o \"" + binary + "\"";

    log << "codegen: compiling: " << cmd << "\n";

    const int rc = std::system(cmd.c_str());  // NOLINT(cert-env33-c)
    if (rc != 0) {
        throw std::runtime_error(
            "compilation failed with exit code " + std::to_string(rc));
    }

    log << "codegen: binary: " << binary << "\n";
}

}  // namespace

// ── register_codegen ──────────────────────────────────────────────────────────

void register_codegen(CLI::App& app) {
    auto cfg = std::make_shared<CodegenConfig>();

    CLI::App* sub = app.add_subcommand(
        "codegen",
        "Generate a standalone native binary from a .gateo design");

    sub->add_option("--output-format", cfg->output_format,
                    "Number format for output values in the compiled binary "
                    "(baked in as the default; overridable at runtime)")
        ->transform(CLI::CheckedTransformer(k_number_format_map, CLI::ignore_case))
        ->default_str("hex");

    // `-d` = directory; avoids `-o` (compiler output file) vs format confusion.
    sub->add_option("-d,--output-dir", cfg->output_dir,
                    "Directory for generated sources and the compiled binary "
                    "(created if missing)")
        ->default_str("gate-build");

    sub->add_option("file", cfg->gateo_path, "Path to the .gateo design file")
        ->required()
        ->check(CLI::ExistingFile);

    sub->callback([cfg] {
        cfg->load_object();
        std::cout << "codegen: loaded \"" << cfg->gateo_path << "\" — "
                  << cfg->gate_object.components.size() << " component(s), "
                  << cfg->gate_object.nodes.size() << " node(s)\n";
        std::cout << "codegen: output-format: "
                  << format_label(cfg->output_format) << "\n";

        const std::string stem =
            std::filesystem::path(cfg->gateo_path).stem().string();
        const std::filesystem::path output_dir = cfg->output_dir;
        std::filesystem::create_directories(output_dir);

        // Emit the design translation unit.
        const auto design_c_path = output_dir / (stem + "_design.c");
        {
            std::ofstream f(design_c_path);
            if (!f) {
                throw std::runtime_error(
                    "cannot create: " + design_c_path.string());
            }
            emit_design_c(cfg->gate_object, cfg->output_format, stem, f);
        }
        std::cout << "codegen: design:  " << design_c_path.string() << "\n";

        compile(output_dir, stem, std::cout);
    });
}

}  // namespace gate_cli::codegen
