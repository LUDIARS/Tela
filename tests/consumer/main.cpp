// @implements SPEC-TL-RUNTIME
// @spec Runtime
#include <tela/runtime.hpp>
#include <tela/transitions.hpp>
int main(){tela::Runtime runtime;tela::Document document;document.button("consumer","Orbis / Iter");runtime.document(std::move(document));return 0;}
