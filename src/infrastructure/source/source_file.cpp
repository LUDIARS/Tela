// @spec SPEC-TL-SOURCE-VIEW
#include <tela/source_view.hpp>
#include <fstream>
#include <stdexcept>

namespace tela {
SourceView load_source(const std::filesystem::path& path, std::size_t target_line) {
    if(!std::filesystem::is_regular_file(path)) throw std::invalid_argument("Source must be a regular file");
    std::ifstream input(path, std::ios::binary);
    if(!input) throw std::runtime_error("Cannot open source file");
    // A bounded read remains bounded even if the file grows after opening.
    std::string bytes(source_max_bytes + 1, '\0');
    input.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    if(input.bad()) throw std::runtime_error("Cannot read source file");
    bytes.resize(static_cast<std::size_t>(input.gcount()));
    const auto name = path.u8string();
    return SourceView(std::string(reinterpret_cast<const char*>(name.data()), name.size()), bytes, target_line);
}
}
