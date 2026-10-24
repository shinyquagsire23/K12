# PATCH_COMMAND for gl4es (deps.cmake), run in its source dir. Idempotent.
#
# framebuffers.c: glBindRenderbuffer(GL_RENDERBUFFER, 0) failed with GL_INVALID_OPERATION (the
# default renderbuffer record has no GLES name), so unbinding never happened -- CAurInternalGL's
# pbuffer setup ends by restoring the saved binding, 0. Binding 0 is always legal.
#
# uniform.c: gl4es fails glUniform*v / glUniformMatrix*v with GL_INVALID_OPERATION when `count`
# exceeds the uniform array's active size. ES 2.0 (2.10.4) and desktop GL ignore the elements past
# the end instead -- and a GLES driver trims an array to its highest used element, so KotorProgram's
# fixed-count uploads (u_lightData x15, u_boneMatrices x51, ...) were dropped whole. Clamp instead.
set(_f "${CMAKE_CURRENT_SOURCE_DIR}/src/gl/uniform.c")
file(READ "${_f}" _s)
if(NOT _s MATCHES "k2: clamp")
  string(REPLACE " || count>m->size) {\n        errorShim(GL_INVALID_OPERATION);\n        return;\n    }"
                 ") {\n        errorShim(GL_INVALID_OPERATION);\n        return;\n    }\n    if(count>m->size) count=m->size;   // k2: clamp (ES 2.0 2.10.4)"
                 _s "${_s}")
  string(REPLACE "  || count>m->size) {\n        errorShim(GL_INVALID_OPERATION);\n        return;\n    }"
                 ") {\n        errorShim(GL_INVALID_OPERATION);\n        return;\n    }\n    if(count>m->size) count=m->size;   // k2: clamp (ES 2.0 2.10.4)"
                 _s "${_s}")
  string(REGEX MATCHALL "k2: clamp" _n "${_s}")
  list(LENGTH _n _n)
  if(NOT _n EQUAL 5)
    message(FATAL_ERROR "gl4es_patch.cmake: expected 5 uniform count checks, patched ${_n}")
  endif()
  file(WRITE "${_f}" "${_s}")
endif()

set(_f "${CMAKE_CURRENT_SOURCE_DIR}/src/gl/framebuffers.c")
file(READ "${_f}" _s)
if(NOT _s MATCHES "k2: unbind")
  set(_old "glrenderbuffer_t * rend = find_renderbuffer(renderbuffer);\n    if(!rend || !rend->renderbuffer) {")
  string(FIND "${_s}" "${_old}" _at)
  if(_at EQUAL -1)
    message(FATAL_ERROR "gl4es_patch.cmake: glBindRenderbuffer check not found")
  endif()
  string(REPLACE "${_old}"
    "glrenderbuffer_t * rend = find_renderbuffer(renderbuffer);\n    if(!rend || (renderbuffer && !rend->renderbuffer)) {   // k2: unbind (0) is legal"
    _s "${_s}")
  file(WRITE "${_f}" "${_s}")
endif()

# texture_params.c: glActiveTexture only records the unit (realize_active() applies it later), but
# glBindTexture binds a cube map at once -- on whatever unit the GLES driver last had active. The
# engine binds its environment map on unit 2 (KotorProgram's samplerCube), so that unit kept a stale
# cube: the bump pass's normalization cube map, read back as a rainbow on every env-mapped surface.
set(_f "${CMAKE_CURRENT_SOURCE_DIR}/src/gl/texture_params.c")
file(READ "${_f}" _s)
if(NOT _s MATCHES "k2: cube unit")
  set(_old "            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:\n                gles_glBindTexture(target, tex?tex->glname:0);")
  string(FIND "${_s}" "${_old}" _at)
  if(_at EQUAL -1)
    message(FATAL_ERROR "gl4es_patch.cmake: glBindTexture cube map case not found")
  endif()
  string(REPLACE "${_old}"
    "            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:\n                realize_active();   // k2: cube unit\n                gles_glBindTexture(target, tex?tex->glname:0);"
    _s "${_s}")
  file(WRITE "${_f}" "${_s}")
endif()

# arbparser.c: ARB `state.fog.params` / `state.fog.color` are unimplemented in gl4es ("Not a valid
# param"), so the conversion of KOTOR 1's skinning vertex program (`PARAM fogParams =
# state.fog.params;`) failed. Map them onto the built-in gl_Fog, which gl4es's GLSL layer provides.
set(_f "${CMAKE_CURRENT_SOURCE_DIR}/src/gl/arbparser.c")
file(READ "${_f}" _s)
if(NOT _s MATCHES "k2: state.fog")
  set(_old "\t\t} else if (!strcmp(tok, \"lightmodel\")) {")
  string(FIND "${_s}" "${_old}" _at)
  if(_at EQUAL -1)
    message(FATAL_ERROR "gl4es_patch.cmake: arbparser lightmodel branch not found")
  endif()
  string(REPLACE "${_old}"
"\t\t} else if (!strcmp(tok, \"fog\")) {   // k2: state.fog
\t\t\tfree(tok);
\t\t\ttok = popFIFO((sArray*)newVar);
\t\t\tif (!tok) {
\t\t\t\treturn NULL;
\t\t\t} else if (!strcmp(tok, \"color\")) {
\t\t\t\tfree(tok);
\t\t\t\tchar **r = (char**)calloc(2, sizeof(char*));
\t\t\t\tr[0] = strdup(\"gl_Fog.color\");
\t\t\t\tr[1] = NULL;
\t\t\t\treturn r;
\t\t\t} else if (!strcmp(tok, \"params\")) {
\t\t\t\tfree(tok);
\t\t\t\tchar **r = (char**)calloc(2, sizeof(char*));
\t\t\t\tr[0] = strdup(\"vec4(gl_Fog.density, gl_Fog.start, gl_Fog.end, gl_Fog.scale)\");
\t\t\t\tr[1] = NULL;
\t\t\t\treturn r;
\t\t\t} else {
\t\t\t\tfree(tok);
\t\t\t\treturn NULL;
\t\t\t}
${_old}" _s "${_s}")
  file(WRITE "${_f}" "${_s}")
endif()

# fpe.c: a vertex program whose ARB->GLSL conversion failed keeps a NULL GLSL source, and the next
# draw with it enabled passed that NULL to fpe_CustomVertexShader (strstr crash). Draw such a program
# with gl4es's default vertex shader instead (fixed-function), as if no vertex program were bound.
set(_f "${CMAKE_CURRENT_SOURCE_DIR}/src/gl/fpe.c")
file(READ "${_f}" _s)
if(NOT _s MATCHES "k2: unconverted")
  set(_old1 "    glstate->fpe->vert = gl4es_glCreateShader(GL_VERTEX_SHADER);\n    if(state->vertex_prg_id) {")
  set(_old2 "        gl4es_glShaderSource(glstate->fpe->vert, 1, fpe_VertexShader(&old_frg->shader->need, state), NULL);")
  string(FIND "${_s}" "${_old1}" _a1)
  string(FIND "${_s}" "${_old2}" _a2)
  if(_a1 EQUAL -1 OR _a2 EQUAL -1)
    message(FATAL_ERROR "gl4es_patch.cmake: fpe_oldprogram vertex branch not found")
  endif()
  string(REPLACE "${_old1}"
    "    glstate->fpe->vert = gl4es_glCreateShader(GL_VERTEX_SHADER);\n    if(state->vertex_prg_id && old_vtx && old_vtx->shader && old_vtx->shader->source) {   // k2: unconverted"
    _s "${_s}")
  string(REPLACE "${_old2}"
    "        gl4es_glShaderSource(glstate->fpe->vert, 1, fpe_VertexShader((old_frg && old_frg->shader) ? &old_frg->shader->need : NULL, state), NULL);"
    _s "${_s}")
  file(WRITE "${_f}" "${_s}")
endif()

# fpe_shader.c: an ARB vertex program drawn with gl4es's default fragment shader must still feed the
# fragment shader's `varying vec4 SecColor`; KOTOR 1's skinning program writes no secondary colour,
# so the GLES link failed ("input SecColor not declared in output from previous stage") and every
# skinned mesh vanished. Declare and zero it when the converted program does not.
set(_f "${CMAKE_CURRENT_SOURCE_DIR}/src/gl/fpe_shader.c")
file(READ "${_f}" _s)
if(NOT _s MATCHES "k2: seccolor")
  set(_a "    int color = default_fragment?(strstr(initial, \"_gl4es_Color\")?0:1):0;")
  set(_b "    // wrap main if needed\n    if(planes || color) {")
  set(_c "        ShadAppend(\"_gl4es_main();\\n\");")
  foreach(_x _a _b _c)
    string(FIND "${_s}" "${${_x}}" _at)
    if(_at EQUAL -1)
      message(FATAL_ERROR "gl4es_patch.cmake: fpe_CustomVertexShader anchor ${_x} not found")
    endif()
  endforeach()
  string(REPLACE "${_a}" "${_a}\n    int seccolor = default_fragment?(strstr(initial, \"SecColor\")?0:1):0;   // k2: seccolor" _s "${_s}")
  string(REPLACE "${_b}" "    if(seccolor) {\n        ShadAppend(\"varying vec4 SecColor;\\n\");\n        ++headline;\n    }\n    // wrap main if needed\n    if(planes || color || seccolor) {" _s "${_s}")
  string(REPLACE "${_c}" "        if(seccolor) ShadAppend(\"SecColor = vec4(0.);\\n\");\n${_c}" _s "${_s}")
  file(WRITE "${_f}" "${_s}")
endif()

# fpe.c: the general form of the SecColor fix. With an ARB vertex program and gl4es's default
# fragment shader, the fragment shader's inputs depend on GL state (FogSrc float/vec3, FogF,
# BackColor, SecBackColor, texcoords ...), and a GLES link fails on any input the converted vertex
# program does not write ("input FogSrc not declared in output from previous stage"). Generate the
# fragment shader first, and append every `varying` it declares that the vertex shader lacks,
# zero-initialised by a wrapping main().
# The built-in outputs it does write are copied into the fragment shader's names (Color <-
# gl_FrontColor, SecColor <- gl_FrontSecondaryColor, FogSrc/FogF <- gl_FogFragCoord,
# _gl4es_TexCoord_N <- the vertex shader's gl_TexCoord[N] array varying): gl4es declares `Color` for
# a custom vertex program but never assigns it, so lit meshes drew a solid colour.
set(_f "${CMAKE_CURRENT_SOURCE_DIR}/src/gl/fpe.c")
file(READ "${_f}" _s)
if(NOT _s MATCHES "k2: missing varyings")
  set(_old "        gl4es_glShaderSource(glstate->fpe->vert, 1, fpe_CustomVertexShader(old_vtx->shader->source, state, state->fragment_prg_id?0:1), NULL);")
  set(_fn "void APIENTRY_GL4ES fpe_oldprogram(fpe_state_t* state) {")
  foreach(_x _old _fn)
    string(FIND "${_s}" "${${_x}}" _at)
    if(_at EQUAL -1)
      message(FATAL_ERROR "gl4es_patch.cmake: fpe.c anchor ${_x} not found")
    endif()
  endforeach()
  string(REPLACE "${_old}" "        k2_set_vertex_source(glstate->fpe->vert, old_vtx, state);" _s "${_s}")
  string(REPLACE "${_fn}" "// k2: missing varyings
#include \"string_utils.h\"
static int k2_declares(const char* vs, const char* name) {
    size_t n = strlen(name);
    for (const char* p = strstr(vs, name); p; p = strstr(p + 1, name)) {
        char b = (p == vs) ? ' ' : p[-1];
        char a = p[n];
        if ((b == ' ' || b == '\\t') && (a == ';' || a == ' ' || a == '[')) {
            const char* l = p;
            while (l > vs && l[-1] != '\\n') l--;
            if (!strncmp(l, \"varying\", 7)) return 1;
        }
    }
    return 0;
}
static void k2_set_vertex_source(GLuint vert, oldprogram_t* old_vtx, fpe_state_t* state) {
    if (state->fragment_prg_id) {
        gl4es_glShaderSource(vert, 1, fpe_CustomVertexShader(old_vtx->shader->source, state, 0), NULL);
        return;
    }
    // fpe_FragmentShader and fpe_CustomVertexShader share one static buffer: copy each result.
    char* frag = strdup(fpe_FragmentShader(&old_vtx->shader->need, state)[0]);
    char* vs = strdup(fpe_CustomVertexShader(old_vtx->shader->source, state, 1)[0]);
    int cap = (int)strlen(vs) + 1;
    int fog = strstr(vs, \"gl_FogFragCoord\") != NULL;
    char decls[4096] = \"\";
    char sets[4096] = \"\";
    for (char* p = strstr(frag, \"varying \"); p; p = strstr(p + 1, \"varying \")) {
        if (p != frag && p[-1] != '\\n') continue;
        char* semi = strchr(p, ';');
        if (!semi || semi - p > 120) continue;
        char line[128];
        memcpy(line, p, semi - p); line[semi - p] = 0;
        char* name = strrchr(line, ' ');
        if (!name) continue;
        *name++ = 0;
        char* type = strrchr(line, ' ');
        if (!type) continue;
        type++;
        const char* src = NULL;
        char buf[160];
        if (!strncmp(name, \"_gl4es_TexCoord_\", 16)) {
            // a vertex shader keeps gl_TexCoord as an ARRAY varying; the default fragment shader
            // reads one varying per unit: copy, swizzled to the fragment side's vecN
            char tc[32];
            snprintf(tc, sizeof tc, \"gl_TexCoord[%s]\", name + 16);
            if (strstr(vs, tc)) {
                const char* sw = !strcmp(type, \"vec2\") ? \".xy\" : !strcmp(type, \"vec3\") ? \".xyz\" : \"\";
                snprintf(buf, sizeof buf, \"%s%s\", tc, sw);
            } else
                snprintf(buf, sizeof buf, \"%s(0.)\", type);
            src = buf;
        } else if (!strcmp(name, \"Color\") || !strcmp(name, \"BackColor\"))
            src = strstr(vs, \"gl_FrontColor\") ? \"gl_FrontColor\" : \"vec4(1.)\";
        else if (!strcmp(name, \"SecColor\") || !strcmp(name, \"SecBackColor\"))
            src = strstr(vs, \"gl_FrontSecondaryColor\") ? \"gl_FrontSecondaryColor\" : \"vec4(0.)\";
        else if (!strcmp(name, \"FogSrc\"))
            src = !fog ? (strcmp(type, \"float\") ? \"vec3(0.)\" : \"0.\")
                       : (strcmp(type, \"float\") ? \"vec3(gl_FogFragCoord, 0., 0.)\" : \"gl_FogFragCoord\");
        else if (!strcmp(name, \"FogF\"))
            src = fog ? \"clamp((gl_Fog.end - gl_FogFragCoord) * gl_Fog.scale, 0., 1.)\" : \"1.\";
        else {
            if (k2_declares(vs, name)) continue;
            snprintf(buf, sizeof buf, strcmp(type, \"float\") ? \"%s(0.)\" : \"0.\", type);
            src = buf;
        }
        if (!k2_declares(vs, name)) {
            if (strlen(decls) + strlen(line) + strlen(name) + 8 >= sizeof decls) break;
            strcat(decls, line); strcat(decls, \" \"); strcat(decls, name); strcat(decls, \";\\n\");
        }
        char z[256];
        snprintf(z, sizeof z, \"%s = %s;\\n\", name, src);
        if (strlen(sets) + strlen(z) >= sizeof sets) break;
        strcat(sets, z);
    }
    if (sets[0]) {
        vs = gl4es_inplace_replace(vs, &cap, \"main\", \"_k2_main\");
        vs = gl4es_append(vs, &cap, \"\\n\");
        vs = gl4es_append(vs, &cap, decls);
        vs = gl4es_append(vs, &cap, \"void main() {\\n_k2_main();\\n\");
        vs = gl4es_append(vs, &cap, sets);
        vs = gl4es_append(vs, &cap, \"}\\n\");
    }
    const char* src = vs;
    if (getenv(\"K2_GL4ES_DUMP\")) {   // log both stages, ~900 bytes per line (logcat truncates longer)
        const char* parts[2] = { vs, frag };
        for (int k = 0; k < 2; k++)
            for (const char* q = parts[k]; *q; q += strlen(q) > 900 ? 900 : strlen(q))
                printf(\"k2dump %s: %.900s\\n\", k ? \"FS\" : \"VS\", q);
    }
    gl4es_glShaderSource(vert, 1, &src, NULL);
    free(vs);
    free(frag);
}

${_fn}" _s "${_s}")
  file(WRITE "${_f}" "${_s}")
endif()

# arbhelper.c: resize() stored the new capacity in bytes (`*cap = newSize`) while every caller
# treats `cap` as an element count, so after the first growth pushArray wrote up to 8x past the
# block. Converting KOTOR 1's ARB programs corrupted the heap (host ASan: heap-buffer-overflow in
# pushArray from parseToken); on the device it surfaced later as random crashes.
set(_f "${CMAKE_CURRENT_SOURCE_DIR}/src/gl/arbhelper.c")
file(READ "${_f}" _s)
if(NOT _s MATCHES "k2: element count")
  set(_old "\t\t*obj = reloc;\n\t\t*cap = newSize;")
  string(FIND "${_s}" "${_old}" _at)
  if(_at EQUAL -1)
    message(FATAL_ERROR "gl4es_patch.cmake: arbhelper resize not found")
  endif()
  string(REPLACE "${_old}" "\t\t*obj = reloc;\n\t\t*cap = newSize / esize;   // k2: element count" _s "${_s}")
  file(WRITE "${_f}" "${_s}")
endif()

# fpe_shader.c: gl4es's default fragment shader, paired with a custom ARB vertex program, turned fog
# on whenever the program writes a fog coordinate and added the secondary colour whenever it writes
# one -- ignoring GL_FOG and GL_COLOR_SUM. KOTOR 1's skinning program writes both (its fog factor
# rides in secondary.x for the port's own GLSL), so with the engine's fog off every body got the
# default GL_EXP / density 1 fog (black) plus a red tint. Gate both on the real GL state.
set(_f "${CMAKE_CURRENT_SOURCE_DIR}/src/gl/fpe_shader.c")
file(READ "${_f}" _s)
if(NOT _s MATCHES "k2: gate fog")
  set(_a "    int fog = is_default?state->fog:need->need_fogcoord;")
  set(_b "    int secondary = is_default?((state->colorsum && !(lighting && light_separate)) || fpe_texenvSecondary(state)):need->need_secondary;")
  foreach(_x _a _b)
    string(FIND "${_s}" "${${_x}}" _at)
    if(_at EQUAL -1)
      message(FATAL_ERROR "gl4es_patch.cmake: fpe_FragmentShader anchor ${_x} not found")
    endif()
  endforeach()
  string(REPLACE "${_a}" "    int fog = is_default?state->fog:(state->fog && need->need_fogcoord);   // k2: gate fog" _s "${_s}")
  string(REPLACE "${_b}" "    int secondary = is_default?((state->colorsum && !(lighting && light_separate)) || fpe_texenvSecondary(state)):(need->need_secondary && (state->colorsum || fpe_texenvSecondary(state)));" _s "${_s}")
  file(WRITE "${_f}" "${_s}")
endif()

# raster.c: glViewport cached the new viewport only when x and y were non-negative, but still sent
# it to GLES. A legal negative-origin viewport (KOTOR 1's dialog panel sets 5,-96 1910x225 and then
# restores 0,0 1920x1080) left the cache on the old value, so the restore matched the stale cache
# and was skipped: the GPU stayed on the negative viewport and the whole touch dialog drew squashed
# into the bottom of the screen. Cache any viewport with a non-negative size. glScissor had the same
# guard (the same replacement fixes both).
set(_f "${CMAKE_CURRENT_SOURCE_DIR}/src/gl/raster.c")
file(READ "${_f}" _s)
if(NOT _s MATCHES "k2: negative origin")
  set(_old "    if (x >= 0 && y >= 0 && width >= 0 && height >= 0) {")
  string(FIND "${_s}" "${_old}" _at)
  if(_at EQUAL -1)
    message(FATAL_ERROR "gl4es_patch.cmake: raster.c viewport cache guard not found")
  endif()
  string(REPLACE "${_old}" "    if (width >= 0 && height >= 0) {   // k2: negative origin is legal" _s "${_s}")
  file(WRITE "${_f}" "${_s}")
endif()


# getter.c: glGetIntegerv / glGetFloatv(GL_ACTIVE_TEXTURE) and GL_CLIENT_ACTIVE_TEXTURE were not
# handled, so they fell through to the GLES driver -- which answers the hardware unit gl4es last
# realized, not the unit the caller selected. A save / switch / restore of the active unit (KOTOR 1's
# fixed-function unit setup in k1kotorgl.cpp) then restored the wrong unit, and the engine's next
# glEnable(GL_TEXTURE_2D) / glBindTexture landed on unit 2 or 3: the HUD drew untextured (white)
# after Carth's dialog. Answer both from gl4es's own state.
set(_f "${CMAKE_CURRENT_SOURCE_DIR}/src/gl/getter.c")
file(READ "${_f}" _s)
if(NOT _s MATCHES "k2: active unit")
  set(_old "        case GL_TEXTURE_BINDING_1D:\n            *params=glstate->texture.bound[glstate->texture.active][ENABLED_TEX1D]?")
  string(FIND "${_s}" "${_old}" _at)
  if(_at EQUAL -1)
    message(FATAL_ERROR "gl4es_patch.cmake: gl4es_commonGet GL_TEXTURE_BINDING_1D case not found")
  endif()
  string(REPLACE "${_old}" "        case 0x84E0:   // k2: active unit (GL_ACTIVE_TEXTURE)\n            *params=0x84C0+glstate->texture.active;\n            break;\n        case 0x84E1:   // GL_CLIENT_ACTIVE_TEXTURE\n            *params=0x84C0+glstate->texture.client;\n            break;\n${_old}" _s "${_s}")
  file(WRITE "${_f}" "${_s}")
endif()

# attributes.h: on Apple, gl4es defines none of its public entry points. Its GNU way is
# __attribute__((alias)) -- glBegin as another name for gl4es_glBegin -- which Mach-O does not
# support, so the Apple branch only declares glBegin and nothing defines it: the engine's direct GL
# calls (glBegin, glTexImage2D, ...) went unresolved. Give Apple the same aliases through the
# assembler (`.set`, as compiler-rt defines its Darwin aliases). iOS only; Android is unaffected.
set(_f "${CMAKE_CURRENT_SOURCE_DIR}/src/gl/attributes.h")
file(READ "${_f}" _s)
if(NOT _s MATCHES "k2: darwin aliases")
  set(_a "#ifndef AliasDecl\n #ifdef __GNUC__")
  set(_b "#ifndef AliasExport\n #if !defined(__EMSCRIPTEN__) && !defined(__APPLE__)")
  foreach(_x _a _b)
    string(FIND "${_s}" "${${_x}}" _at)
    if(_at EQUAL -1)
      message(FATAL_ERROR "gl4es_patch.cmake: attributes.h anchor ${_x} not found")
    endif()
  endforeach()
  string(REPLACE "${_a}" "#ifndef AliasDecl
 #if defined(__APPLE__)   // k2: darwin aliases
  #define _K2_DARWIN_ALIAS(NEW,OLD) __asm__(\".globl _\" NEW \"\\n.set _\" NEW \", _\" OLD);
  #define AliasDecl(RET,NAME,DEF,OLD) _K2_DARWIN_ALIAS(_STR(NAME),_STR(OLD)) \\
   RET APIENTRY_GL4ES NAME DEF
 #elif defined(__GNUC__)" _s "${_s}")
  string(REPLACE "${_b}" "#ifndef AliasExport
 #if defined(__APPLE__) && !defined(__EMSCRIPTEN__)   // k2: darwin aliases
  #define _AliasExport_(RET,ENM,DEF,INM,SUF) _K2_DARWIN_ALIAS(_STR(ENM),_STR(gl4es_##INM)) \\
      RET APIENTRY_GL4ES ENM DEF
  #define NonAliasExportDecl(RET,NAME,DEF) RET APIENTRY_GL4ES NAME DEF
  #define AliasExport(RET,NAME,X,DEF)       _AliasExport_(RET,NAME##X,DEF,NAME,0)
  #define AliasExport_A(RET,NAME,X,DEF,INM) _AliasExport_(RET,NAME##X,DEF,INM,0)
  #define AliasExport_D(RET,NAME,X,DEF)     _AliasExport_(RET,NAME##X,DEF,NAME,0)
  #define AliasExport_D_1(RET,NAME,X,DEF)   _AliasExport_(RET,NAME##X,DEF,NAME,0)
  #define AliasExport_M(RET,NAME,X,DEF,SUF) _AliasExport_(RET,NAME##X,DEF,NAME,0)
  #define AliasExport_V(RET,NAME)           _AliasExport_(RET,NAME,(void),NAME,0)
 #elif !defined(__EMSCRIPTEN__) && !defined(__APPLE__)" _s "${_s}")
  file(WRITE "${_f}" "${_s}")
endif()

# program.c: each element of a uniform array gets its own record, at "the base location + j". ES 2.0
# does not make an array's element locations consecutive, and Apple's driver does not: in a program
# gl4es customises for fixed-function state (fpe_CustomShader, e.g. a texture unit's texgen/texenv),
# fpe_SyncUniforms re-uploads the parent's uniforms element by element, and u_matrices[1] -- the MVP
# -- went to a location that is not it: GL_INVALID_OPERATION, the matrix stayed zero, and the draw
# rasterised nothing (KOTOR II's chargen Portrait head on iOS; Android's drivers number elements
# consecutively). Ask the driver for "name[j]".
set(_f "${CMAKE_CURRENT_SOURCE_DIR}/src/gl/program.c")
file(READ "${_f}" _s)
if(NOT _s MATCHES "k2: element locations")
  set(_old "            if(id!=-1) {\n                for (int j = 0; j<size; j++) {\n                    k = kh_put(uniformlist, uniforms, id, &ret);")
  string(FIND "${_s}" "${_old}" _at)
  if(_at EQUAL -1)
    message(FATAL_ERROR "gl4es_patch.cmake: program.c uniform loop not found")
  endif()
  string(REPLACE "${_old}" "            if(id!=-1) {\n                GLint k2id0 = id;   // k2: element locations\n                for (int j = 0; j<size; j++) {\n                    if(j) {   // k2: element locations -- ES 2.0 does not make them consecutive\n                        char k2elem[256];\n                        snprintf(k2elem, sizeof(k2elem), \"%s[%d]\", name, j);\n                        GLint k2id = gles_glGetUniformLocation(glprogram->id, k2elem);\n                        id = (k2id!=-1) ? k2id : k2id0+j;\n                    }\n                    k = kh_put(uniformlist, uniforms, id, &ret);" _s "${_s}")
  file(WRITE "${_f}" "${_s}")
endif()

# getter.c: gl4es exports glGetString under the plain name, and so does the shim (shim_kotorgl.cpp:
# the GL_EXTENSIONS a GLES driver reports, which is what the engine's direct calls must read). gl4es
# finds its own entry through gl_lookup.c's table (gl4es_glGetString), never by the plain name, so the
# alias goes and the program has one glGetString.
set(_f "${CMAKE_CURRENT_SOURCE_DIR}/src/gl/getter.c")
file(READ "${_f}" _s)
if(NOT _s MATCHES "k2: glGetString")
  set(_old "AliasExport(const GLubyte*,glGetString,,(GLenum name));")
  string(FIND "${_s}" "${_old}" _at)
  if(_at EQUAL -1)
    message(FATAL_ERROR "gl4es_patch.cmake: getter.c glGetString export not found")
  endif()
  string(REPLACE "${_old}" "// k2: glGetString is the shim's (shim_kotorgl.cpp); gl_lookup.c maps the name to gl4es_glGetString" _s "${_s}")
  file(WRITE "${_f}" "${_s}")
endif()
