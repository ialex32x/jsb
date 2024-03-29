#ifndef JAVASCRIPT_SOURCE_MAP_H
#define JAVASCRIPT_SOURCE_MAP_H
#include "jsb_macros.h"

namespace jsb::internal
{
    struct SourcePosition
    {
        int index = 0;
        int line = 0;
        int column = 0;
    };

    struct SourceMap
    {
    private:
        struct InternalPosition {
            // DO NOT CHANGE THE ORDER
            int result_column = 0;
            int source_index = 0;
            int source_line = 0;
            int source_column = 0;
            int _reserved = 0; // name index
            jsb_force_inline int& operator[](uint8_t index) { jsb_check(index < 5); return *((int*)this + index); }
        };
        struct InternalLine
        {
            int result_line = 0;
            Vector<InternalPosition> elements;
        };
        Vector<InternalLine> source_lines_;
        Vector<String> sources_;
        String source_root_;

    public:
        // input string is `mappings` (not the whole json), example: `;;;AAAA,iCAA6B;AAC7B,MAAa,QAAQ;CAAI`
        bool parse_mappings(const char* p_mapings, size_t p_len);

        // parse sourcemap json string
        bool parse(const String& p_source_map);

        // input: js source position [line, column]
        // output: ts source position
        //NOTE line & column are both zero-based
        bool find(int p_line, int p_column, SourcePosition& r_pos) const;

        const String& get_source_root() const;
        const String& get_source(int index) const;

    private:
        void decode(int p_line, const char* p_token, const char* p_end, InternalPosition& r_pos, int& r_aline, int& r_acol);
        InternalLine& operator[](int p_line);
    };
}
#endif
