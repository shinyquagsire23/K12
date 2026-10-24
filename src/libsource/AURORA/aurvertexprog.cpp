// ARB/NV vertex-program translation unit.

#include "k2stdint.h"
#include "port/aslglapi.h"
#if KOTOR1
#define K1_AURORA_LIST  // K1 builds its own AURORA list; List.h grows from 8.
#endif
#include "aurvertexprog.h"
#include "List.h"
#include "AURORA/aurmodel.h"
#include "aurlightmanager.h"
#include "aurinternal.h"
#include "aurscene.h"


char cBumpyShinyVP[] = "!!VP1.0 # This is the vertex program for doing the bumpy-shiny stuff.\nDP4   o[HPOS].x, c[0], v[OPOS];DP4   o[HPOS].y, c[1], v[OPOS];DP4   o[HPOS].z, c[2], v[OPOS];DP4   o[HPOS].w, c[3], v[OPOS];DP3   R1.x, c[8], v[TEX4]; DP3   R2.x, c[8], v[TEX5]; DP3   R3.x, c[8], v[TEX6]; DP3   R1.y, c[9], v[TEX4]; DP3   R2.y, c[9], v[TEX5]; DP3   R3.y, c[9], v[TEX6]; DP3   R1.z, c[10], v[TEX4]; DP3   R2.z, c[10], v[TEX5]; DP3   R3.z, c[10], v[TEX6]; DP3   o[TEX1].x, c[92], R1; DP3   o[TEX1].y, c[92], R2; DP3   o[TEX1].z, c[92], R3; DP3   o[TEX2].x, c[91], R1; DP3   o[TEX2].y, c[91], R2; DP3   o[TEX2].z, c[91], R3; DP3   o[TEX3].x, c[90], R1; DP3   o[TEX3].y, c[90], R2; DP3   o[TEX3].z, c[90], R3; DP4   R4.x, v[OPOS], c[8] ;DP4   R4.y, v[OPOS], c[9] ;DP4   R4.z, v[OPOS], c[10] ;DP4   R4.w, v[OPOS], c[11] ;DP3   R1.w, R4, R4;RSQ   R1.w, R1.w;RCP   R1.w, R1.w;MOV o[FOGC], R1.w;RCP   R4.w, R4.w           ;MUL   R4.xyz, R4, -R4.w    ;MOV   o[TEX0], v[TEX0];DP3   o[TEX1].w, R4, c[92];DP3   o[TEX2].w, R4, c[91];DP3   o[TEX3].w, R4, c[90];MOV   o[COL0], c[15].x;";
char cBumpyShinySkinVP[] = "!!VP1.0 # This is the vertex program for skinned animations.\nMUL   R0, v[COL1], c[16].zzzz;ARL   A0.x, R0.x;DP4   R1.x, v[OPOS], c[A0.x + 18];DP4   R1.y, v[OPOS], c[A0.x + 19];DP4   R1.z, v[OPOS], c[A0.x + 20];MUL   R9, v[WGHT].x, R1;DP3   R1.x, v[TEX4], c[A0.x + 18];DP3   R1.y, v[TEX4], c[A0.x + 19];DP3   R1.z, v[TEX4], c[A0.x + 20];MUL   R4, v[WGHT].x, R1;DP3   R1.x, v[TEX5], c[A0.x + 18];DP3   R1.y, v[TEX5], c[A0.x + 19];DP3   R1.z, v[TEX5], c[A0.x + 20];MUL   R5, v[WGHT].x, R1;DP3   R1.x, v[TEX6], c[A0.x + 18];DP3   R1.y, v[TEX6], c[A0.x + 19];DP3   R1.z, v[TEX6], c[A0.x + 20];MUL   R6, v[WGHT].x, R1;ARL   A0.x, R0.y;DP4   R1.x, v[OPOS], c[A0.x + 18];DP4   R1.y, v[OPOS], c[A0.x + 19];DP4   R1.z, v[OPOS], c[A0.x + 20];MAD   R9, R1, v[WGHT].y, R9;DP3   R1.x, v[TEX4], c[A0.x + 18];DP3   R1.y, v[TEX4], c[A0.x + 19];DP3   R1.z, v[TEX4], c[A0.x + 20];MAD   R4, v[WGHT].y, R1, R4;DP3   R1.x, v[TEX5], c[A0.x + 18];DP3   R1.y, v[TEX5], c[A0.x + 19];DP3   R1.z, v[TEX5], c[A0.x + 20];MAD   R5, v[WGHT].y, R1, R5;DP3   R1.x, v[TEX6], c[A0.x + 18];DP3   R1.y, v[TEX6], c[A0.x + 19];DP3   R1.z, v[TEX6], c[A0.x + 20];MAD   R6, v[WGHT].y, R1, R6;ARL   A0.x, R0.z;DP4   R1.x, v[OPOS], c[A0.x + 18];DP4   R1.y, v[OPOS], c[A0.x + 19];DP4   R1.z, v[OPOS], c[A0.x + 20];MAD   R9, R1, v[WGHT].z, R9;DP3   R1.x, v[TEX4], c[A0.x + 18];DP3   R1.y, v[TEX4], c[A0.x + 19];DP3   R1.z, v[TEX4], c[A0.x + 20];MAD   R4, v[WGHT].z, R1, R4;DP3   R1.x, v[TEX5], c[A0.x + 18];DP3   R1.y, v[TEX5], c[A0.x + 19];DP3   R1.z, v[TEX5], c[A0.x + 20];MAD   R5, v[WGHT].z, R1, R5;DP3   R1.x, v[TEX6], c[A0.x + 18];DP3   R1.y, v[TEX6], c[A0.x + 19];DP3   R1.z, v[TEX6], c[A0.x + 20];MAD   R6, v[WGHT].z, R1, R6;ARL   A0.x, R0.w;DP4   R1.x, v[OPOS], c[A0.x + 18];DP4   R1.y, v[OPOS], c[A0.x + 19];DP4   R1.z, v[OPOS], c[A0.x + 20];MAD   R9, R1, v[WGHT].w, R9;DP3   R1.x, v[TEX4], c[A0.x + 18];\nDP3   R1.y, v[TEX4], c[A0.x + 19];\nDP3   R1.z, v[TEX4], c[A0.x + 20];\nMAD   R4, v[WGHT].w, R1, R4;DP3   R1.x, v[TEX5], c[A0.x + 18];\nDP3   R1.y, v[TEX5], c[A0.x + 19];\nDP3   R1.z, v[TEX5], c[A0.x + 20];\nMAD   R5, v[WGHT].w, R1, R5;DP3   R1.x, v[TEX6], c[A0.x + 18];\nDP3   R1.y, v[TEX6], c[A0.x + 19];\nDP3   R1.z, v[TEX6], c[A0.x + 20];\nMAD   R6, v[WGHT].w, R1, R5;MOV   R9.w, c[16].w;DP4   o[HPOS].x, c[0], R9;DP4   o[HPOS].y, c[1], R9;DP4   o[HPOS].z, c[2], R9;DP4   o[HPOS].w, c[3], R9;DP4   R10.x, c[8], R9;DP4   R10.y, c[9], R9;DP4   R10.z, c[10], R9;MOV   R10.w, c[16].w;MOV   R9, c[16].xxxw;DP3   R1.x, c[8], R4; DP3   R2.x, c[8], R5; DP3   R3.x, c[8], R6; DP3   R1.y, c[9], R4; DP3   R2.y, c[9], R5; DP3   R3.y, c[9], R6; DP3   R1.z, c[10], R4; DP3   R2.z, c[10], R5; DP3   R3.z, c[10], R6; DP3   o[TEX1].x, c[92], R1; DP3   o[TEX1].y, c[92], R2; DP3   o[TEX1].z, c[92], R3; DP3   o[TEX2].x, c[91], R1; DP3   o[TEX2].y, c[91], R2; DP3   o[TEX2].z, c[91], R3; DP3   o[TEX3].x, c[90], R1; DP3   o[TEX3].y, c[90], R2; DP3   o[TEX3].z, c[90], R3; DP4   R4.x, v[OPOS], c[8] ;DP4   R4.y, v[OPOS], c[9] ;DP4   R4.z, v[OPOS], c[10] ;DP4   R4.w, v[OPOS], c[11] ;RCP   R4.w, R4.w           ;MUL   R4.xyz, R4, -R4.w    ;DP3   R1.w, R4, R4;RSQ   R1.w, R1.w;RCP   R1.w, R1.w;MOV o[FOGC], R1.w;MOV   o[TEX0], v[TEX0];DP3   o[TEX1].w, R4, c[92];DP3   o[TEX2].w, R4, c[91];DP3   o[TEX3].w, R4, c[90];MOV   o[COL0], c[15].x;";
char cBumpMapOnePassVP[] = "!!VP1.0 # This is the vertex program for transforming light/halfangle vectors into tangent space.\nDP4   o[HPOS].x, c[0], v[OPOS];DP4   o[HPOS].y, c[1], v[OPOS];DP4   o[HPOS].z, c[2], v[OPOS];DP4   o[HPOS].w, c[3], v[OPOS];ADD  R0, c[12], -v[OPOS];DP3   R0.w, R0, R0;RSQ   R1.w, R0.w;DST R2, R0.w, R1.w;DP3 R2.w, R2, c[14];RCP R2.w, R2.w;MOV   R4.xyzw, R2.wwww;MUL   R0, R0, R1.w;MOV   R0.w, c[17].x;DP3   R1.x, v[TEX4], R0;DP3   R1.y, v[TEX5], R0;DP3   R1.z, v[TEX6], R0;MOV   R1.w, c[17].x;MOV   o[TEX1], R1;ADD R2, c[13],-v[OPOS];DP3   R2.w, R2, R2;RSQ   R2.w, R2.w;MUL   R2, R2, R2.w;MOV   R2.w, c[17].x;ADD R2, R2,R0;MOV   R2.w, c[17].x;DP3   R1.x, v[TEX4], R2;DP3   R1.y, v[TEX5], R2;DP3   R1.z, v[TEX6], R2;MOV   R1.w, c[17].x;MOV   o[TEX2], R1;DP4   R10.x, c[8], v[OPOS];DP4   R10.y, c[9], v[OPOS];DP4   R10.z, c[10], v[OPOS];DP4   R10.w, c[11], v[OPOS];DP3   R11.x, c[8], v[NRML];DP3   R11.y, c[9], v[NRML];DP3   R11.z, c[10], v[NRML];DP3   R1.w, R10, R10;RSQ   R1.w, R1.w;RCP   R1.w, R1.w;MOV   o[FOGC], R1.w;MOV   R9, v[COL0];MOV   o[TEX0], v[TEX0];MOV   o[TEX3], v[TEX0];MOV   o[COL1], R4;";
char cBumpMapSpecularVP[] = "!!VP1.0 # This is the vertex program for transforming light/halfangle vectors into tangent space.\nDP4   o[HPOS].x, c[0], v[OPOS];DP4   o[HPOS].y, c[1], v[OPOS];DP4   o[HPOS].z, c[2], v[OPOS];DP4   o[HPOS].w, c[3], v[OPOS];ADD  R0, c[12], -v[OPOS];DP3   R0.w, R0, R0;RSQ   R1.w, R0.w;MUL   R0, R0, R1.w;DST R2, R0.w, R1.w;DP3 R2.w, R2, c[14];RCP R2.w, R2.w;MOV   R4.xyzw, R2.wwww;ADD R2, c[13],-v[OPOS];DP3   R2.w, R2, R2;RSQ   R2.w, R2.w;MUL   R2, R2, R2.w;MOV   R2.w, c[17].x;ADD R2, R2,R0;MOV   R2.w, c[17].x;DP3   R1.x, v[TEX4], R2;DP3   R1.y, v[TEX5], R2;DP3   R1.z, v[TEX6], R2;MOV   R1.w, c[17].x;MOV   o[TEX1], R1;DP4   R10.x, c[8], v[OPOS];DP4   R10.y, c[9], v[OPOS];DP4   R10.z, c[10], v[OPOS];DP4   R10.w, c[11], v[OPOS];DP3   R11.x, c[8], v[NRML];DP3   R11.y, c[9], v[NRML];DP3   R11.z, c[10], v[NRML];DP3   R1.w, R10, R10;RSQ   R1.w, R1.w;RCP   R1.w, R1.w;MOV   o[FOGC], R1.w;MOV   o[COL1], R4;MOV   R9, v[COL0];MOV   o[TEX0], v[TEX0];MOV   o[TEX2], v[TEX1];MOV   o[TEX3], v[TEX0];";
char cBumpMapDiffuseVP[] = "!!VP1.0 # This is the vertex program for transforming light/halfangle vectors into tangent space.\nDP4   o[HPOS].x, c[0], v[OPOS];DP4   o[HPOS].y, c[1], v[OPOS];DP4   o[HPOS].z, c[2], v[OPOS];DP4   o[HPOS].w, c[3], v[OPOS];ADD  R0, c[12], -v[OPOS];DP3   R0.w, R0, R0;RSQ   R1.w, R0.w;DST R2, R0.w, R1.w;DP3 R2.w, R2, c[14];RCP R2.w, R2.w;MOV   R4.xyzw, R2.wwww;MUL   R0, R0, R1.w;MOV   R0.w, c[17].x;DP3   R1.x, v[TEX4], R0;DP3   R1.y, v[TEX5], R0;DP3   R1.z, v[TEX6], R0;MOV   R1.w, c[17].x;MOV   o[TEX1], R1;DP4   R10.x, c[8], v[OPOS];DP4   R10.y, c[9], v[OPOS];DP4   R10.z, c[10], v[OPOS];DP4   R10.w, c[11], v[OPOS];DP3   R11.x, c[8], v[NRML];DP3   R11.y, c[9], v[NRML];DP3   R11.z, c[10], v[NRML];DP3   R1.w, R10, R10;RSQ   R1.w, R1.w;RCP   R1.w, R1.w;MOV o[FOGC], R1.w;MOV   o[COL1], R4;MOV   R9, v[COL0];MOV   o[TEX0], v[TEX0];MOV   o[TEX2], v[TEX1];MOV   o[TEX3], v[TEX0];";
char cBumpMapVP[] = "!!VP1.0 # This is the vertex program for transforming light/halfangle vectors into tangent space.\nDP4   o[HPOS].x, c[0], v[OPOS];DP4   o[HPOS].y, c[1], v[OPOS];DP4   o[HPOS].z, c[2], v[OPOS];DP4   o[HPOS].w, c[3], v[OPOS];MOV   R0, v[TEX7];DP3   R1.x, v[TEX4], R0;DP3   R1.y, v[TEX5], R0;DP3   R1.z, v[TEX6], R0;MOV   R1.w, c[17].x;MOV   o[TEX1], R1;MOV   R9, v[COL0];MOV   o[COL0], v[COL0];MOV   o[FOGC], v[FOGC];MOV   o[TEX0], v[TEX0];";
char cStandardVP[] = "!!VP1.0 # This is the standard vertex program.\nDP4   o[HPOS].x, c[0], v[OPOS];DP4   o[HPOS].y, c[1], v[OPOS];DP4   o[HPOS].z, c[2], v[OPOS];DP4   o[HPOS].w, c[3], v[OPOS];DP4   R10.x, c[8], v[OPOS];DP4   R10.y, c[9], v[OPOS];DP4   R10.z, c[10], v[OPOS];DP4   R10.w, c[11], v[OPOS];DP3   R1.w, R10, R10;RSQ   R1.w, R1.w;RCP   R1.w, R1.w;MOV   o[FOGC], R1.w;DP3   R11.x, c[8], v[NRML];DP3   R11.y, c[9], v[NRML];DP3   R11.z, c[10], v[NRML];MOV R9, c[15].wwwz;MOV   o[TEX0], v[TEX0];MOV   o[TEX1], v[TEX1];MOV   o[TEX2], v[TEX2];MOV   o[TEX3], v[TEX3];MOV o[COL0], c[15].wwwz;";
#if !KOTOR1
char cStandardDirtyVP[] = "!!VP1.0 # This is the standard vertex program.\nDP4\to[HPOS].x, c[0], v[OPOS];DP4\to[HPOS].y, c[1], v[OPOS];DP4\to[HPOS].z, c[2], v[OPOS];DP4\to[HPOS].w, c[3], v[OPOS];DP4   R10.x, c[8], v[OPOS];DP4   R10.y, c[9], v[OPOS];DP4   R10.z, c[10], v[OPOS];DP4   R10.w, c[11], v[OPOS];DP4\tR11.x, R10, c[92];DP4\tR11.y, R10, C[91];DP4\tR11.z, R10, C[90];DP4\tR11.w, R10, C[89];MAD\tR11, R11, C[28].x, R11;\nMAD\tR11, R11, C[28].y, R11.yzxw;\nMAD\tR11, R11, C[28].z, R11.zxyw;\nMAD\to[TEX1], R11, C[28].w, R11;\nDP3\tR1.w, R10, R10;RSQ\tR1.w, R1.w;RCP\tR1.w, R1.w;MOV\to[FOGC], R1.w;DP3\tR11.x, c[8], v[NRML];DP3\tR11.y, c[9], v[NRML];DP3\tR11.z, c[10], v[NRML];MOV\tR9, c[15].wwwz;MOV   o[TEX0], v[TEX0];MOV   o[TEX2], v[TEX2];MOV   o[TEX3], v[TEX3];MOV o[COL0], c[15].wwwz;";
char cStandardDirtyLMVP[] = "!!VP1.0 # This is the standard vertex program.\nDP4\to[HPOS].x, c[0], v[OPOS];DP4\to[HPOS].y, c[1], v[OPOS];DP4\to[HPOS].z, c[2], v[OPOS];DP4\to[HPOS].w, c[3], v[OPOS];DP4   R10.x, c[8], v[OPOS];DP4   R10.y, c[9], v[OPOS];DP4   R10.z, c[10], v[OPOS];DP4   R10.w, c[11], v[OPOS];DP4\tR11.x, R10, c[92];DP4\tR11.y, R10, C[91];DP4\tR11.z, R10, C[90];DP4\tR11.w, R10, C[89];MAD\tR11, R11, C[28].x, R11;\nMAD\tR11, R11, C[28].y, R11.yzxw;\nMAD\tR11, R11, C[28].z, R11.zxyw;\nMAD\to[TEX2], R11, C[28].w, R11;\nDP3\tR1.w, R10, R10;RSQ\tR1.w, R1.w;RCP\tR1.w, R1.w;MOV\to[FOGC], R1.w;DP3\tR11.x, c[8], v[NRML];DP3\tR11.y, c[9], v[NRML];DP3\tR11.z, c[10], v[NRML];MOV\tR9, c[15].wwwz;MOV   o[TEX0], v[TEX0];MOV   o[TEX1], v[TEX1];MOV   o[TEX3], v[TEX3];MOV o[COL0], c[15].wwwz;";
char cStandardDirtyEMVP[] = "!!VP1.0 # This is the standard vertex program.\nDP4\to[HPOS].x, c[0], v[OPOS];DP4\to[HPOS].y, c[1], v[OPOS];DP4\to[HPOS].z, c[2], v[OPOS];DP4\to[HPOS].w, c[3], v[OPOS];DP4   R10.x, c[8], v[OPOS];DP4   R10.y, c[9], v[OPOS];DP4   R10.z, c[10], v[OPOS];DP4   R10.w, c[11], v[OPOS];DP4\tR11.x, R10, c[92];DP4\tR11.y, R10, C[91];DP4\tR11.z, R10, C[90];DP4\tR11.w, R10, C[89];MAD\tR11, R11, C[28].x, R11;\nMAD\tR11, R11, C[28].y, R11.yzxw;\nMAD\tR11, R11, C[28].z, R11.zxyw;\nMAD\to[TEX2], R11, C[28].w, R11;\nDP3\tR1.w, R10, R10;RSQ\tR1.w, R1.w;RCP\tR1.w, R1.w;MOV\to[FOGC], R1.w;DP3\tR11.x, c[8], v[NRML];DP3\tR11.y, c[9], v[NRML];DP3\tR11.z, c[10], v[NRML];MOV\tR9, c[15].wwwz;MOV   o[TEX0], v[TEX0];DP3   R0.x, -R10, R11;MUL   R1, R0.x, R11;MUL   R2, R1, c[16].y;ADD   R3, R10, R2;DP3   R3.w, R3, R3;RSQ   R3.w, R3.w;MUL   R3, R3, R3.w;DP3   R4.x, c[92], R3;DP3   R4.y, c[91], R3;DP3   R4.z, c[90], R3;MOV   R4.w, c[16].w;MOV   o[TEX1], R4;";
char cStandardDirtyEMLMVP[] = "!!VP1.0 # This is the standard vertex program.\nDP4\to[HPOS].x, c[0], v[OPOS];DP4\to[HPOS].y, c[1], v[OPOS];DP4\to[HPOS].z, c[2], v[OPOS];DP4\to[HPOS].w, c[3], v[OPOS];DP4   R10.x, c[8], v[OPOS];DP4   R10.y, c[9], v[OPOS];DP4   R10.z, c[10], v[OPOS];DP4   R10.w, c[11], v[OPOS];DP4\tR11.x, R10, c[92];DP4\tR11.y, R10, C[91];DP4\tR11.z, R10, C[90];DP4\tR11.w, R10, C[89];MAD\tR11, R11, C[28].x, R11;\nMAD\tR11, R11, C[28].y, R11.yzxw;\nMAD\tR11, R11, C[28].z, R11.zxyw;\nMAD\to[TEX3], R11, C[28].w, R11;\nDP3\tR1.w, R10, R10;RSQ\tR1.w, R1.w;RCP\tR1.w, R1.w;MOV\to[FOGC], R1.w;DP3\tR11.x, c[8], v[NRML];DP3\tR11.y, c[9], v[NRML];DP3\tR11.z, c[10], v[NRML];MOV\tR9, c[15].wwwz;MOV\to[TEX0], v[TEX0];MOV\to[TEX1], v[TEX1];DP3   R0.x, -R10, R11;MUL   R1, R0.x, R11;MUL   R2, R1, c[16].y;ADD   R3, R10, R2;DP3   R3.w, R3, R3;RSQ   R3.w, R3.w;MUL   R3, R3, R3.w;DP3   R4.x, c[92], R3;DP3   R4.y, c[91], R3;DP3   R4.z, c[90], R3;MOV   R4.w, c[16].w;MOV   o[TEX2], R4;";
char cStandardDirtyLSVP[] = "!!VP1.0 # This is the standard vertex program.\nDP4\to[HPOS].x, c[0], v[OPOS];DP4\to[HPOS].y, c[1], v[OPOS];DP4\to[HPOS].z, c[2], v[OPOS];DP4\to[HPOS].w, c[3], v[OPOS];DP4   R10.x, c[8], v[OPOS];DP4   R10.y, c[9], v[OPOS];DP4   R10.z, c[10], v[OPOS];DP4   R10.w, c[11], v[OPOS];MOV\tR11, v[OPOS];ADD\tR11, R11, v[TEX0];MOV\to[TEX1], R11;DP3\tR1.w, R10, R10;RSQ\tR1.w, R1.w;RCP\tR1.w, R1.w;MOV\to[FOGC], R1.w;DP3\tR11.x, c[8], v[NRML];DP3\tR11.y, c[9], v[NRML];DP3\tR11.z, c[10], v[NRML];MOV\tR9, c[15].wwwz;MOV   o[TEX0], v[TEX0];";
char cStandardDirtyLSLMVP[] = "!!VP1.0 # This is the standard vertex program.\nDP4\to[HPOS].x, c[0], v[OPOS];DP4\to[HPOS].y, c[1], v[OPOS];DP4\to[HPOS].z, c[2], v[OPOS];DP4\to[HPOS].w, c[3], v[OPOS];DP4   R10.x, c[8], v[OPOS];DP4   R10.y, c[9], v[OPOS];DP4   R10.z, c[10], v[OPOS];DP4   R10.w, c[11], v[OPOS];MOV\tR11, v[OPOS];ADD\tR11, R11, v[TEX0];MOV\to[TEX2], R11;DP3\tR1.w, R10, R10;RSQ\tR1.w, R1.w;RCP\tR1.w, R1.w;MOV\to[FOGC], R1.w;DP3\tR11.x, c[8], v[NRML];DP3\tR11.y, c[9], v[NRML];DP3\tR11.z, c[10], v[NRML];MOV\tR9, c[15].wwwz;MOV   o[TEX0], v[TEX0];MOV   o[TEX1], v[TEX1];";
char cStandardDirtyLSEMVP[] = "!!VP1.0 # This is the standard vertex program.\nDP4\to[HPOS].x, c[0], v[OPOS];DP4\to[HPOS].y, c[1], v[OPOS];DP4\to[HPOS].z, c[2], v[OPOS];DP4\to[HPOS].w, c[3], v[OPOS];DP4   R10.x, c[8], v[OPOS];DP4   R10.y, c[9], v[OPOS];DP4   R10.z, c[10], v[OPOS];DP4   R10.w, c[11], v[OPOS];MOV\tR11, v[OPOS];ADD\tR11, R11, v[TEX0];MOV\to[TEX2], R11;DP3\tR1.w, R10, R10;RSQ\tR1.w, R1.w;RCP\tR1.w, R1.w;MOV\to[FOGC], R1.w;DP3\tR11.x, c[8], v[NRML];DP3\tR11.y, c[9], v[NRML];DP3\tR11.z, c[10], v[NRML];MOV\tR9, c[15].wwwz;MOV   o[TEX0], v[TEX0];DP3   R0.x, -R10, R11;MUL   R1, R0.x, R11;MUL   R2, R1, c[16].y;ADD   R3, R10, R2;DP3   R3.w, R3, R3;RSQ   R3.w, R3.w;MUL   R3, R3, R3.w;DP3   R4.x, c[92], R3;DP3   R4.y, c[91], R3;DP3   R4.z, c[90], R3;MOV   R4.w, c[16].w;MOV   o[TEX1], R4;";
char cStandardDirtyLSEMLMVP[] = "!!VP1.0 # This is the standard vertex program.\nDP4\to[HPOS].x, c[0], v[OPOS];DP4\to[HPOS].y, c[1], v[OPOS];DP4\to[HPOS].z, c[2], v[OPOS];DP4\to[HPOS].w, c[3], v[OPOS];DP4   R10.x, c[8], v[OPOS];DP4   R10.y, c[9], v[OPOS];DP4   R10.z, c[10], v[OPOS];DP4   R10.w, c[11], v[OPOS];MOV\tR11, v[OPOS];ADD\tR11, R11, v[TEX0];MOV\to[TEX3], R11;DP3\tR1.w, R10, R10;RSQ\tR1.w, R1.w;RCP\tR1.w, R1.w;MOV\to[FOGC], R1.w;DP3\tR11.x, c[8], v[NRML];DP3\tR11.y, c[9], v[NRML];DP3\tR11.z, c[10], v[NRML];MOV\tR9, c[15].wwwz;MOV   o[TEX0], v[TEX0];MOV   o[TEX1], v[TEX1];DP3   R0.x, -R10, R11;MUL   R1, R0.x, R11;MUL   R2, R1, c[16].y;ADD   R3, R10, R2;DP3   R3.w, R3, R3;RSQ   R3.w, R3.w;MUL   R3, R3, R3.w;DP3   R4.x, c[92], R3;DP3   R4.y, c[91], R3;DP3   R4.z, c[90], R3;MOV   R4.w, c[16].w;MOV   o[TEX2], R4;";
#endif
char cFogOutputVP[] = "!!VP1.0 # This is the fog output vertex program.\nDP4   o[HPOS].x, c[0], v[OPOS];DP4   o[HPOS].y, c[1], v[OPOS];DP4   o[HPOS].z, c[2], v[OPOS];DP4   o[HPOS].w, c[3], v[OPOS];DP4   R1.x, c[8], v[OPOS];DP4   R1.y, c[9], v[OPOS];DP4   R1.z, c[10], v[OPOS];DP4   R1.w, c[11], v[OPOS];DP3   R1.w, R1, R1;RSQ   R1.w, R1.w;RCP   R1.w, R1.w;ADD   R2.x, c[20].y, -c[20].x;ADD   R2.y, c[20].y, -R1.w;RCP   R2.z, R2.x;MUL   R2.w, R2.y, R2.z;MIN   R2.w, R2.w, c[22].x;MAX   R2.w, R2.w, c[22].y;ADD   R2.w, c[22].x, -R2.w;MUL   R3, c[21], R2.w;MOV   R3.w, R2.w;MOV   o[COL0], R3;MOV   o[FOGC].x, c[22].y;";
char cSkinVP[] = "!!VP1.0 # This is the vertex program for skinned animations.\nMUL   R0, v[COL1], c[16].zzzz;ARL   A0.x, R0.x;DP4   R1.x, v[OPOS], c[A0.x + 18];DP4   R1.y, v[OPOS], c[A0.x + 19];DP4   R1.z, v[OPOS], c[A0.x + 20];DP3   R5.x, v[NRML], c[A0.x + 18];DP3   R5.y, v[NRML], c[A0.x + 19];DP3   R5.z, v[NRML], c[A0.x + 20];ARL   A0.x, R0.y;DP4   R2.x, v[OPOS], c[A0.x + 18];DP4   R2.y, v[OPOS], c[A0.x + 19];DP4   R2.z, v[OPOS], c[A0.x + 20];DP3   R6.x, v[NRML], c[A0.x + 18];DP3   R6.y, v[NRML], c[A0.x + 19];DP3   R6.z, v[NRML], c[A0.x + 20];ARL   A0.x, R0.z;DP4   R3.x, v[OPOS], c[A0.x + 18];DP4   R3.y, v[OPOS], c[A0.x + 19];DP4   R3.z, v[OPOS], c[A0.x + 20];DP3   R7.x, v[NRML], c[A0.x + 18];DP3   R7.y, v[NRML], c[A0.x + 19];DP3   R7.z, v[NRML], c[A0.x + 20];ARL   A0.x, R0.w;DP4   R4.x, v[OPOS], c[A0.x + 18];DP4   R4.y, v[OPOS], c[A0.x + 19];DP4   R4.z, v[OPOS], c[A0.x + 20];DP3   R8.x, v[NRML], c[A0.x + 18];DP3   R8.y, v[NRML], c[A0.x + 19];DP3   R8.z, v[NRML], c[A0.x + 20];MUL   R9, v[WGHT].x, R1;MAD   R9, R2, v[WGHT].y, R9;MAD   R9, R3, v[WGHT].z, R9;MAD   R9, R4, v[WGHT].w, R9;MOV   R9.w, c[16].w;MUL   R10, v[WGHT].x, R5;MAD   R10, R6, v[WGHT].y, R10;MAD   R10, R7, v[WGHT].z, R10;MAD   R10, R8, v[WGHT].w, R10;DP4   o[HPOS].x, c[0], R9;DP4   o[HPOS].y, c[1], R9;DP4   o[HPOS].z, c[2], R9;DP4   o[HPOS].w, c[3], R9;DP3   R11.x, c[8], R10;DP3   R11.y, c[9], R10;DP3   R11.z, c[10], R10;MOV   R11.w, c[16].w;DP3   R11.w, R11, R11;RSQ   R11.w, R11.w;MUL   R11, R11, R11.w;DP4   R10.x, c[8], R9;DP4   R10.y, c[9], R9;DP4   R10.z, c[10], R9;MOV   R10.w, c[16].w;DP3   R1.w, R10, R10;RSQ   R1.w, R1.w;RCP   R1.w, R1.w;MOV o[FOGC], R1.w;MOV   R9, c[16].xxxw;MOV   o[TEX0], v[TEX0];";
char cSkinBumpedOutVP[] = "!!VP1.0 # This is the vertex program for skinned animations.\nMUL   R0, v[COL1], c[16].zzzz;ARL   A0.x, R0.x;DP4   R1.x, v[OPOS], c[A0.x + 18];DP4   R1.y, v[OPOS], c[A0.x + 19];DP4   R1.z, v[OPOS], c[A0.x + 20];DP3   R5.x, v[NRML], c[A0.x + 18];DP3   R5.y, v[NRML], c[A0.x + 19];DP3   R5.z, v[NRML], c[A0.x + 20];ARL   A0.x, R0.y;DP4   R2.x, v[OPOS], c[A0.x + 18];DP4   R2.y, v[OPOS], c[A0.x + 19];DP4   R2.z, v[OPOS], c[A0.x + 20];DP3   R6.x, v[NRML], c[A0.x + 18];DP3   R6.y, v[NRML], c[A0.x + 19];DP3   R6.z, v[NRML], c[A0.x + 20];ARL   A0.x, R0.z;DP4   R3.x, v[OPOS], c[A0.x + 18];DP4   R3.y, v[OPOS], c[A0.x + 19];DP4   R3.z, v[OPOS], c[A0.x + 20];DP3   R7.x, v[NRML], c[A0.x + 18];DP3   R7.y, v[NRML], c[A0.x + 19];DP3   R7.z, v[NRML], c[A0.x + 20];ARL   A0.x, R0.w;DP4   R4.x, v[OPOS], c[A0.x + 18];DP4   R4.y, v[OPOS], c[A0.x + 19];DP4   R4.z, v[OPOS], c[A0.x + 20];DP3   R8.x, v[NRML], c[A0.x + 18];DP3   R8.y, v[NRML], c[A0.x + 19];DP3   R8.z, v[NRML], c[A0.x + 20];MUL   R9, v[WGHT].x, R1;MAD   R9, R2, v[WGHT].y, R9;MAD   R9, R3, v[WGHT].z, R9;MAD   R9, R4, v[WGHT].w, R9;MOV   R9.w, c[16].w;MUL   R10, v[WGHT].x, R5;MAD   R10, R6, v[WGHT].y, R10;MAD   R10, R7, v[WGHT].z, R10;MAD   R10, R8, v[WGHT].w, R10;MAD R9, R10, c[12].x, R9;DP4   o[HPOS].x, c[0], R9;DP4   o[HPOS].y, c[1], R9;DP4   o[HPOS].z, c[2], R9;DP4   o[HPOS].w, c[3], R9;DP3   R11.x, c[8], R10;DP3   R11.y, c[9], R10;DP3   R11.z, c[10], R10;MOV   R11.w, c[16].w;DP3   R11.w, R11, R11;RSQ   R11.w, R11.w;MUL   R11, R11, R11.w;DP4   R10.x, c[8], R9;DP4   R10.y, c[9], R9;DP4   R10.z, c[10], R9;MOV   R10.w, c[16].w;DP3   R1.w, R10, R10;RSQ   R1.w, R1.w;RCP   R1.w, R1.w;MOV o[FOGC], R1.w;MOV   R9, c[12].wwww;MOV   o[COL0].xyzw, c[12].wwww;MOV   o[TEX0], v[TEX0];";
char cSkin3BoneVP[] = "!!VP1.0 # This is the vertex program for skinned animations.\nMUL   R0, v[COL1], c[16].zzzz;ARL   A0.x, R0.x;DP4   R1.x, v[OPOS], c[A0.x + 18];DP4   R1.y, v[OPOS], c[A0.x + 19];DP4   R1.z, v[OPOS], c[A0.x + 20];DP3   R5.x, v[NRML], c[A0.x + 18];DP3   R5.y, v[NRML], c[A0.x + 19];DP3   R5.z, v[NRML], c[A0.x + 20];ARL   A0.x, R0.y;DP4   R2.x, v[OPOS], c[A0.x + 18];DP4   R2.y, v[OPOS], c[A0.x + 19];DP4   R2.z, v[OPOS], c[A0.x + 20];DP3   R6.x, v[NRML], c[A0.x + 18];DP3   R6.y, v[NRML], c[A0.x + 19];DP3   R6.z, v[NRML], c[A0.x + 20];ARL   A0.x, R0.z;DP4   R3.x, v[OPOS], c[A0.x + 18];DP4   R3.y, v[OPOS], c[A0.x + 19];DP4   R3.z, v[OPOS], c[A0.x + 20];DP3   R7.x, v[NRML], c[A0.x + 18];DP3   R7.y, v[NRML], c[A0.x + 19];DP3   R7.z, v[NRML], c[A0.x + 20];ARL   A0.x, R0.w;DP4   R4.x, v[OPOS], c[A0.x + 18];DP4   R4.y, v[OPOS], c[A0.x + 19];DP4   R4.z, v[OPOS], c[A0.x + 20];DP3   R8.x, v[NRML], c[A0.x + 18];DP3   R8.y, v[NRML], c[A0.x + 19];DP3   R8.z, v[NRML], c[A0.x + 20];MUL   R9, v[WGHT].x, R1;MAD   R9, R2, v[WGHT].y, R9;MAD   R9, R3, v[WGHT].z, R9;MAD   R9, R4, v[WGHT].w, R9;MOV   R9.w, c[16].w;MUL   R10, v[WGHT].x, R5;MAD   R10, R6, v[WGHT].y, R10;MAD   R10, R7, v[WGHT].z, R10;MAD   R10, R8, v[WGHT].w, R10;DP4   o[HPOS].x, c[0], R9;DP4   o[HPOS].y, c[1], R9;DP4   o[HPOS].z, c[2], R9;DP4   o[HPOS].w, c[3], R9;DP3   R11.x, c[8], R10;DP3   R11.y, c[9], R10;DP3   R11.z, c[10], R10;MOV   R11.w, c[16].w;DP3   R11.w, R11, R11;RSQ   R11.w, R11.w;MUL   R11, R11, R11.w;DP4   R10.x, c[8], R9;DP4   R10.y, c[9], R9;DP4   R10.z, c[10], R9;MOV   R10.w, c[16].w;DP3   R1.w, R10, R10;RSQ   R1.w, R1.w;RCP   R1.w, R1.w;MOV o[FOGC], R1.w;MOV   R9, c[16].xxxw;MOV   o[TEX0], v[TEX0];";
char cSkinBumpyVP[] = "!!VP1.0 # This is the vertex program for skinned animations.\nMUL   R0, v[COL1], c[16].zzzz;ARL   A0.x, R0.x;DP4   R1.x, v[OPOS], c[A0.x + 18];DP4   R1.y, v[OPOS], c[A0.x + 19];DP4   R1.z, v[OPOS], c[A0.x + 20];DP3   R5.x, v[NRML], c[A0.x + 18];DP3   R5.y, v[NRML], c[A0.x + 19];DP3   R5.z, v[NRML], c[A0.x + 20];ARL   A0.x, R0.y;DP4   R2.x, v[OPOS], c[A0.x + 18];DP4   R2.y, v[OPOS], c[A0.x + 19];DP4   R2.z, v[OPOS], c[A0.x + 20];DP3   R6.x, v[NRML], c[A0.x + 18];DP3   R6.y, v[NRML], c[A0.x + 19];DP3   R6.z, v[NRML], c[A0.x + 20];ARL   A0.x, R0.z;DP4   R3.x, v[OPOS], c[A0.x + 18];DP4   R3.y, v[OPOS], c[A0.x + 19];DP4   R3.z, v[OPOS], c[A0.x + 20];DP3   R7.x, v[NRML], c[A0.x + 18];DP3   R7.y, v[NRML], c[A0.x + 19];DP3   R7.z, v[NRML], c[A0.x + 20];ARL   A0.x, R0.w;DP4   R4.x, v[OPOS], c[A0.x + 18];DP4   R4.y, v[OPOS], c[A0.x + 19];DP4   R4.z, v[OPOS], c[A0.x + 20];DP3   R8.x, v[NRML], c[A0.x + 18];DP3   R8.y, v[NRML], c[A0.x + 19];DP3   R8.z, v[NRML], c[A0.x + 20];MUL   R9, v[WGHT].x, R1;MAD   R9, R2, v[WGHT].y, R9;MAD   R9, R3, v[WGHT].z, R9;MAD   R9, R4, v[WGHT].w, R9;MOV   R9.w, c[16].w;MUL   R10, v[WGHT].x, R5;MAD   R10, R6, v[WGHT].y, R10;MAD   R10, R7, v[WGHT].z, R10;MAD   R10, R8, v[WGHT].w, R10;DP4   o[HPOS].x, c[0], R9;DP4   o[HPOS].y, c[1], R9;DP4   o[HPOS].z, c[2], R9;DP4   o[HPOS].w, c[3], R9;DP3   R11.x, c[8], R10;DP3   R11.y, c[9], R10;DP3   R11.z, c[10], R10;MOV   R11.w, c[16].w;DP3   R11.w, R11, R11;RSQ   R11.w, R11.w;MUL   R11, R11, R11.w;DP4   R10.x, c[8], R9;DP4   R10.y, c[9], R9;DP4   R10.z, c[10], R9;MOV   R10.w, c[16].w;ADD  R0, c[12], -R9;DP3   R0.w, R0, R0;RSQ   R1.w, R0.w;DST R2, R0.w, R1.w;DP3 R2.w, R2, c[14];RCP R2.w, R2.w;MOV   R4.xyzw, R2.wwww;MUL   R0, R0, R1.w;MOV   R0.w, c[16].w;ADD R2, c[13],-R9;DP3   R2.w, R2, R2;RSQ   R2.w, R2.w;MUL   R2, R2, R2.w;MOV   R2.w, c[16].w;ADD R2, R2,R0;MOV   R2.w, c[16].w;MOV   R9, c[16].xxxw;MOV   o[TEX0], v[TEX0];MOV   o[TEX3], v[TEX0];DP3   R1.x, v[TEX4], R0;DP3   R1.y, v[TEX5], R0;DP3   R1.z, v[TEX6], R0;MOV   R1.w, c[16].w;MOV   o[TEX1], R1;DP3   R1.x, v[TEX4], R2;DP3   R1.y, v[TEX5], R2;DP3   R1.z, v[TEX6], R2;MOV   R1.w, c[16].w;MOV   o[TEX2], R1;DP3   R1.w, R10, R10;RSQ   R1.w, R1.w;RCP   R1.w, R1.w;MOV   o[FOGC], R1.w;MOV   o[COL1], R4;";
char cNewSkinBumpyVP[] = "!!VP1.0 # This is the vertex program for skinned animations.\nMUL   R0, v[COL1], c[16].zzzz;ARL   A0.x, R0.x;DP4   R1.x, v[OPOS], c[A0.x + 18];DP4   R1.y, v[OPOS], c[A0.x + 19];DP4   R1.z, v[OPOS], c[A0.x + 20];MUL   R9, v[WGHT].x, R1;DP3   R1.x, v[TEX4], c[A0.x + 18];DP3   R1.y, v[TEX4], c[A0.x + 19];DP3   R1.z, v[TEX4], c[A0.x + 20];MUL   R4, v[WGHT].x, R1;DP3   R1.x, v[TEX5], c[A0.x + 18];DP3   R1.y, v[TEX5], c[A0.x + 19];DP3   R1.z, v[TEX5], c[A0.x + 20];MUL   R5, v[WGHT].x, R1;DP3   R1.x, v[TEX6], c[A0.x + 18];DP3   R1.y, v[TEX6], c[A0.x + 19];DP3   R1.z, v[TEX6], c[A0.x + 20];MUL   R6, v[WGHT].x, R1;ARL   A0.x, R0.y;DP4   R1.x, v[OPOS], c[A0.x + 18];DP4   R1.y, v[OPOS], c[A0.x + 19];DP4   R1.z, v[OPOS], c[A0.x + 20];MAD   R9, R1, v[WGHT].y, R9;DP3   R1.x, v[TEX4], c[A0.x + 18];DP3   R1.y, v[TEX4], c[A0.x + 19];DP3   R1.z, v[TEX4], c[A0.x + 20];MAD   R4, R1, v[WGHT].y, R4;DP3   R1.x, v[TEX5], c[A0.x + 18];DP3   R1.y, v[TEX5], c[A0.x + 19];DP3   R1.z, v[TEX5], c[A0.x + 20];MAD   R5, R1, v[WGHT].y, R5;DP3   R1.x, v[TEX6], c[A0.x + 18];DP3   R1.y, v[TEX6], c[A0.x + 19];DP3   R1.z, v[TEX6], c[A0.x + 20];MAD   R6, R1, v[WGHT].y, R6;ARL   A0.x, R0.z;DP4   R1.x, v[OPOS], c[A0.x + 18];DP4   R1.y, v[OPOS], c[A0.x + 19];DP4   R1.z, v[OPOS], c[A0.x + 20];MAD   R9, R1, v[WGHT].z, R9;DP3   R1.x, v[TEX4], c[A0.x + 18];DP3   R1.y, v[TEX4], c[A0.x + 19];DP3   R1.z, v[TEX4], c[A0.x + 20];MAD   R4, R1, v[WGHT].z, R4;DP3   R1.x, v[TEX5], c[A0.x + 18];DP3   R1.y, v[TEX5], c[A0.x + 19];DP3   R1.z, v[TEX5], c[A0.x + 20];MAD   R5, R1, v[WGHT].z, R5;DP3   R1.x, v[TEX6], c[A0.x + 18];DP3   R1.y, v[TEX6], c[A0.x + 19];DP3   R1.z, v[TEX6], c[A0.x + 20];MAD   R6, R1, v[WGHT].z, R6;ARL   A0.x, R0.w;DP4   R1.x, v[OPOS], c[A0.x + 18];DP4   R1.y, v[OPOS], c[A0.x + 19];DP4   R1.z, v[OPOS], c[A0.x + 20];MAD   R9, R1, v[WGHT].w, R9;DP3   R1.x, v[TEX4], c[A0.x + 18];DP3   R1.y, v[TEX4], c[A0.x + 19];DP3   R1.z, v[TEX4], c[A0.x + 20];MAD   R4, R1, v[WGHT].w, R4;DP3   R1.x, v[TEX5], c[A0.x + 18];DP3   R1.y, v[TEX5], c[A0.x + 19];DP3   R1.z, v[TEX5], c[A0.x + 20];MAD   R5, R1, v[WGHT].w, R5;DP3   R1.x, v[TEX6], c[A0.x + 18];DP3   R1.y, v[TEX6], c[A0.x + 19];DP3   R1.z, v[TEX6], c[A0.x + 20];MAD   R6, R1, v[WGHT].w, R6;MOV   R9.w, c[16].w;DP4   o[HPOS].x, c[0], R9;DP4   o[HPOS].y, c[1], R9;DP4   o[HPOS].z, c[2], R9;DP4   o[HPOS].w, c[3], R9;DP4   R10.x, c[8], R9;DP4   R10.y, c[9], R9;DP4   R10.z, c[10], R9;MOV   R10.w, c[16].w;DP3   R1.w, R10, R10;RSQ   R1.w, R1.w;RCP   R1.w, R1.w;MOV   o[FOGC], R1.w;ADD  R0, c[12], -R9;DP3   R0.w, R0, R0;RSQ   R1.w, R0.w;DST R2, R0.w, R1.w;DP3 R2.w, R2, c[14];RCP R2.w, R2.w;MOV   o[COL1].xyzw, R2.wwww;MUL   R0, R0, R1.w;MOV   R0.w, c[16].w;ADD R2, c[13],-R9;DP3   R2.w, R2, R2;RSQ   R2.w, R2.w;MUL   R2, R2, R2.w;MOV   R2.w, c[16].w;ADD R2, R2,R0;MOV   R2.w, c[16].w;MOV   R9, c[16].xxxw;MOV   o[TEX0], v[TEX0];MOV   o[TEX3], v[TEX0];DP3   R1.x, R4, R0;DP3   R1.y, R5, R0;DP3   R1.z, R6, R0;MOV   R1.w, c[16].w;MOV   o[TEX1], R1;DP3   R1.x, R4, R2;DP3   R1.y, R5, R2;DP3   R1.z, R6, R2;MOV   R1.w, c[16].w;MOV   o[TEX2], R1;";
char cSkinCubeMapVP[] = "!!VP1.0 # This is the vertex program for skinned animations.\nMUL   R0, v[COL1], c[16].zzzz;ARL   A0.x, R0.x;DP4   R1.x, v[OPOS], c[A0.x + 18];DP4   R1.y, v[OPOS], c[A0.x + 19];DP4   R1.z, v[OPOS], c[A0.x + 20];DP3   R5.x, v[NRML], c[A0.x + 18];DP3   R5.y, v[NRML], c[A0.x + 19];DP3   R5.z, v[NRML], c[A0.x + 20];ARL   A0.x, R0.y;DP4   R2.x, v[OPOS], c[A0.x + 18];DP4   R2.y, v[OPOS], c[A0.x + 19];DP4   R2.z, v[OPOS], c[A0.x + 20];DP3   R6.x, v[NRML], c[A0.x + 18];DP3   R6.y, v[NRML], c[A0.x + 19];DP3   R6.z, v[NRML], c[A0.x + 20];ARL   A0.x, R0.z;DP4   R3.x, v[OPOS], c[A0.x + 18];DP4   R3.y, v[OPOS], c[A0.x + 19];DP4   R3.z, v[OPOS], c[A0.x + 20];DP3   R7.x, v[NRML], c[A0.x + 18];DP3   R7.y, v[NRML], c[A0.x + 19];DP3   R7.z, v[NRML], c[A0.x + 20];ARL   A0.x, R0.w;DP4   R4.x, v[OPOS], c[A0.x + 18];DP4   R4.y, v[OPOS], c[A0.x + 19];DP4   R4.z, v[OPOS], c[A0.x + 20];DP3   R8.x, v[NRML], c[A0.x + 18];DP3   R8.y, v[NRML], c[A0.x + 19];DP3   R8.z, v[NRML], c[A0.x + 20];MUL   R9, v[WGHT].x, R1;MAD   R9, R2, v[WGHT].y, R9;MAD   R9, R3, v[WGHT].z, R9;MAD   R9, R4, v[WGHT].w, R9;MOV   R9.w, c[16].w;MUL   R10, v[WGHT].x, R5;MAD   R10, R6, v[WGHT].y, R10;MAD   R10, R7, v[WGHT].z, R10;MAD   R10, R8, v[WGHT].w, R10;DP4   o[HPOS].x, c[0], R9;DP4   o[HPOS].y, c[1], R9;DP4   o[HPOS].z, c[2], R9;DP4   o[HPOS].w, c[3], R9;DP3   R11.x, c[8], R10;DP3   R11.y, c[9], R10;DP3   R11.z, c[10], R10;MOV   R11.w, c[16].w;DP3   R11.w, R11, R11;RSQ   R11.w, R11.w;MUL   R11, R11, R11.w;DP4   R10.x, c[8], R9;DP4   R10.y, c[9], R9;DP4   R10.z, c[10], R9;MOV   R10.w, c[16].w;DP3   R1.w, R10, R10;RSQ   R1.w, R1.w;RCP   R1.w, R1.w;MOV o[FOGC], R1.w;MOV   R9, c[16].xxxw;MOV   o[TEX0], v[TEX0];DP3   R0.x, -R10, R11;MUL   R1, R0.x, R11;MUL   R2, R1, c[16].y;ADD   R3, R10, R2;DP3   R3.w, R3, R3;RSQ   R3.w, R3.w;MUL   R3, R3, R3.w;DP3   R4.x, c[92], R3;DP3   R4.y, c[91], R3;DP3   R4.z, c[90], R3;MOV   R4.w, c[16].w;MOV   o[TEX1], R4;";
char cSkinMotionBlurVP[] = "!!VP1.0 # This is the standard vertex program.\nMUL   R0, v[COL1], c[16].zzzz;ARL   A0.x, R0.x;DP4   R1.x, v[OPOS], c[A0.x + 18];DP4   R1.y, v[OPOS], c[A0.x + 19];DP4   R1.z, v[OPOS], c[A0.x + 20];DP3   R5.x, v[NRML], c[A0.x + 18];DP3   R5.y, v[NRML], c[A0.x + 19];DP3   R5.z, v[NRML], c[A0.x + 20];ARL   A0.x, R0.y;DP4   R2.x, v[OPOS], c[A0.x + 18];DP4   R2.y, v[OPOS], c[A0.x + 19];DP4   R2.z, v[OPOS], c[A0.x + 20];DP3   R6.x, v[NRML], c[A0.x + 18];DP3   R6.y, v[NRML], c[A0.x + 19];DP3   R6.z, v[NRML], c[A0.x + 20];ARL   A0.x, R0.z;DP4   R3.x, v[OPOS], c[A0.x + 18];DP4   R3.y, v[OPOS], c[A0.x + 19];DP4   R3.z, v[OPOS], c[A0.x + 20];DP3   R7.x, v[NRML], c[A0.x + 18];DP3   R7.y, v[NRML], c[A0.x + 19];DP3   R7.z, v[NRML], c[A0.x + 20];ARL   A0.x, R0.w;DP4   R4.x, v[OPOS], c[A0.x + 18];DP4   R4.y, v[OPOS], c[A0.x + 19];DP4   R4.z, v[OPOS], c[A0.x + 20];DP3   R8.x, v[NRML], c[A0.x + 18];DP3   R8.y, v[NRML], c[A0.x + 19];DP3   R8.z, v[NRML], c[A0.x + 20];MUL   R9, v[WGHT].x, R1;MAD   R9, R2, v[WGHT].y, R9;MAD   R9, R3, v[WGHT].z, R9;MAD   R9, R4, v[WGHT].w, R9;MOV   R9.w, c[16].w;MUL   R10, v[WGHT].x, R5;MAD   R10, R6, v[WGHT].y, R10;MAD   R10, R7, v[WGHT].z, R10;MAD   R10, R8, v[WGHT].w, R10;DP3   R10.w, R10, R10;RSQ   R10.w, R10.w;MUL   R10,   R10, R10.w;MOV   R10.w, c[16].x;MOV  R4, c[12];MOV   R7.x, c[14].x;MIN   R2.y, R7.x, c[14].y;MUL   R5.xyz, R4, R2.y;MOV   R5.w, c[16].x;DP3   R3.w, R4, R10;SLT   R2.w, R3.w, c[16].x;MOV   R1, c[17].xyzw;MOV   R1.w, c[16].x;MAD   R6, R2.w, -R5, R9.xyzw;MUL   R7.y, R2.y, c[13].x;RCP   R8.y, c[14].y;MUL   R8.w, R2.y, R8.y;MUL   R7.y, R7.y, R8.w;MUL   R7.y, R7.y, c[17].w;MOV   R3.w,   c[16].x;MOV   R3.xyz, c[69];DP3   R3.w,   R3, R3;RSQ   R5.w,   R3.w;MUL   R3,     R3, R5.w;ADD   R5.xyz, R6, -R1;DP3   R5.w, R5, R3;SLT   R5.x, R5.w, c[16].x;MOV   R7.x, c[16].w;MAD   R5.y, R5.x, -c[16].y, R7.x;MUL   R5.z, R5.y, R7.y;MUL   R5.z, R5.z, R2.w;MAD   R6, R5.z, R3, R6;ADD   R2.x, -R2.w, c[16].w;MUL   R1, R10, -c[14].z;MOV   R1.w, c[16].x;MAD   R5, R2.x, R1, R6;DP4   R1.x, R5, c[0];DP4   R1.y, R5, c[1];DP4   R1.z, R5, c[2];DP4   R1.w, R5, c[3];MOV   o[HPOS].xyzw, R1.xyzw;ADD   R2.w, c[16].w, -R2.w;MOV   R9.xyz, c[16].xxxx;MUL   R9.w, c[69].w, R2.w;MOV   o[COL0], R9;DP3   R11.x, c[8],  R10;DP3   R11.y, c[9],  R10;DP3   R11.z, c[10], R10;MOV   R11.w, c[16].w;DP4   R10.x, c[8],  R5;DP4   R10.y, c[9],  R5;DP4   R10.z, c[10], R5;MOV   R10.w, c[16].w;DP3   R1.w, R10, R10;RSQ   R1.w, R1.w;RCP   R1.w, R1.w;MOV   o[FOGC].x, -R1.w;MOV   o[TEX0], v[TEX0];";
char cBumpedOutVP[] = "!!VP1.0 # This is the bumped-out vertex program.\nMOV\tR1, v[OPOS];MOV\tR2, v[NRML];MAD R1, R2, c[12].xxxx, R1;DP4   o[HPOS].x, c[0], R1;DP4   o[HPOS].y, c[1], R1;DP4   o[HPOS].z, c[2], R1;DP4   o[HPOS].w, c[3], R1;DP4   R10.x, c[8], R1;DP4   R10.y, c[9], R1;DP4   R10.z, c[10], R1;DP4   R10.w, c[11], R1;DP3   R1.w, R10, R10;RSQ   R1.w, R1.w;RCP   R1.w, R1.w;MOV   o[FOGC], R1.w;DP3   R11.x, c[8], v[NRML];DP3   R11.y, c[9], v[NRML];DP3   R11.z, c[10], v[NRML];MOV   R9, v[COL0];MOV   o[TEX0], v[TEX0];MOV   o[TEX1], v[TEX1];MOV o[COL0], c[12].wwww;";
unsigned int* cBumpyShinyRegularVPDeclaration;
unsigned int* cBumpyShinyLightMappedVPDeclaration;
unsigned int* cBumpyShinySkinVPDeclaration;
unsigned int* cBumpMapOnePassVPDeclaration;
unsigned int* cBumpMapSpecularVPDeclaration;
unsigned int* cBumpMapDiffuseVPDeclaration;
unsigned int* cBumpMapVPDeclaration;
unsigned int* cStandardVPDeclaration;
unsigned int* cStandardVP2Declaration;
unsigned int* cFogOutputVPDeclaration;
unsigned int* cSkinVPDeclaration;
unsigned int* cSkinBumpedOutVPDeclaration;
unsigned int* cSkin3BoneVPDeclaration;
unsigned int* cSkinBumpyVPDeclaration;
unsigned int* cSkinSphereMapVPDeclaration;
unsigned int* cSkinCubeMapVPDeclaration;
unsigned int* cSkinMotionBlurVPDeclaration;

extern int usebumpdiffuse;
extern int usebumpspecular;
extern int usenewbumpyskin;
extern int renderskin;

bool AurNonRadeon8500Validate();
void AurGetVertexProgramARBError(char* pszProgram);

#include "port/libcbase.h"
#include "port/libcstring.h"

extern int usearbvertexprograms;
class Scene;

// Frame serial that Enable compares its install stamp against; AurVertexProgramMakeCurrent bumps it.
static int currentframe;

List<VertexProgram*> VertexProgramManager::cm_listExistentVertexPrograms;

void TrackMatrices()
{
    if (usearbvertexprograms)
        ASLgl::glBindProgramARB(GL_VERTEX_PROGRAM_ARB, 0);
}

void VertexProgramManager::AddVertexProgram(VertexProgram* vp)
{
    cm_listExistentVertexPrograms.Add(vp);
}

void VertexProgramManager::RemoveVertexProgram(VertexProgram* vp)
{
    cm_listExistentVertexPrograms.Remove(vp);
}

void VertexProgramManager::ResetVertexPrograms()
{
    for (int i = 0; i < cm_listExistentVertexPrograms.m_count; i++) {
        ASLgl::glDeleteProgramsARB(1, &cm_listExistentVertexPrograms.m_pData[i]->m_uProgramARB);
        cm_listExistentVertexPrograms.m_pData[i]->m_uProgramARB = 0;
        cm_listExistentVertexPrograms.m_pData[i]->m_bProgramLoaded = 0;
    }
}

#if KOTOR1  // K1 scans the light budget up front, then pins it to 3
VertexProgram::VertexProgram(char* program, unsigned int* declaration)
    : m_pProgramText(program),
      m_pDeclaration(declaration),
      m_uProgramARB(0),
      m_nAllowedLights(0),
      m_nInstalledStamp(-1)
{
    FindNumberOfAllowedLights();
    m_bProgramLoaded = 0;
    m_nAllowedLights = 3;
    VertexProgramManager::AddVertexProgram(this);
}
#else
VertexProgram::VertexProgram(char* program, unsigned int* declaration)
    : m_pProgramText(program),
      m_pDeclaration(declaration),
      m_uProgramARB(0),
      m_nAllowedLights(3),
      m_nInstalledStamp(-1),
      m_bProgramLoaded(0)
{
    VertexProgramManager::AddVertexProgram(this);
}
#endif

// Releases the GL program when vertex programs are in use.
VertexProgram::~VertexProgram()
{
#if KOTOR1  // K1 deletes the program without a glIsProgram test
    if (usearbvertexprograms != 0)
        ASLgl::glDeleteProgramsARB(1, &m_uProgramARB);
#else
    unsigned int* pProgramARB = &m_uProgramARB;
    if (usearbvertexprograms != 0 && (ASLgl::glIsProgramARB(*pProgramARB) & 0xff) != 0) {
        if (usearbvertexprograms != 0)
            ASLgl::glDeleteProgramsARB(1, pProgramARB);
    }
#endif
    VertexProgramManager::RemoveVertexProgram(this);
}

void VertexProgram::FindNumberOfAllowedLights()
{
#if KOTOR1  // K1 re-reads the length every step; the budget is min((0x72 - n) / 14, 3)
    int nSemi = 0;
    for (int i = 0; i < static_cast<int>(strlen(m_pProgramText)); i++) {
        if (m_pProgramText[i] == ';')
            nSemi++;
    }
    int nLights = (114 - nSemi) / 14;
    m_nAllowedLights = (nLights < 3) ? nLights : 3;
#else
    int nLen = static_cast<int>(strlen(m_pProgramText));
    int nSemi = 0;

    for (int i = 0; i < nLen; i++) {
        if (m_pProgramText[i] == ';')
            nSemi++;
    }

    m_nAllowedLights = 3;
    if (nSemi > 58)
        m_nAllowedLights = (114 - nSemi) / 14;
#endif
}

// Builds the program text from the vertex-program source plus the light block, uploads it, and
// binds the result. SetUpVertexProgramLightShaderConstants is called twice on purpose.
void VertexProgram::Enable(Part* part, bool flag)
{
    static char* lastProg;
    static unsigned char wasLit;

    unsigned int* pProgramARB = &m_uProgramARB;

#if KOTOR1  // K1 omits the "program named" term and the glIsProgram test, and reaches its light
            // manager at a different Scene offset.
    if (lastProg == m_pProgramText
        && LightManager::cm_bLightManagerUpdated == 0
        && wasLit == (flag & 1)
        && AurNonRadeon8500Validate() != 0)
        goto BIND;
#else
    if (*pProgramARB != 0
        && lastProg == m_pProgramText
        && LightManager::cm_bLightManagerUpdated == 0
        && wasLit == (flag & 1)
        && AurNonRadeon8500Validate() != 0)
        goto BIND;
#endif

    LightManager::cm_bLightManagerUpdated = 0;
    wasLit = flag & 1;
    lastProg = m_pProgramText;

    if (part != 0 && (flag & 1))
        currentscene->m_pLightManager->SetUpVertexProgramLightShaderConstants(m_nAllowedLights,
                                                                             part->m_pGob == 0);

    if (m_nInstalledStamp != currentframe) {
#if KOTOR1
        if ((flag & 1) == 0)
            goto BIND;
#else
        if (usearbvertexprograms != 0
            && (ASLgl::glIsProgramARB(*pProgramARB) & 0xff) != 0
            && (flag & 1) == 0)
            goto BIND;
#endif

        if (m_bProgramLoaded != 0 && CAurInternal::m_nHeadLightEnabled == 0) {
            if (usearbvertexprograms != 0)
                goto BIND;
            return;
        }

        m_nInstalledStamp = currentframe;
        __strcpy_chk(m_szProgram, m_pProgramText, 8192);

        if (flag & 1) {
            if (m_nAllowedLights > 0) {
                char* pLights = new char[4096];
                *pLights = 0;
                LightManager::SetupVertexProgramLights(pLights, m_nAllowedLights,
                                                       part != 0 && part->m_pGob == 0);
                __strcat_chk(m_szProgram, pLights, 8192);
                delete[] pLights;
            }
        }

        __strcat_chk(m_szProgram, "END", 8192);

        if (*pProgramARB == 0 && usearbvertexprograms != 0)
            ASLgl::glGenProgramsARB(1, pProgramARB);
        if (usearbvertexprograms != 0)
            ASLgl::glBindProgramARB(GL_VERTEX_PROGRAM_ARB, *pProgramARB);

        int nLen = static_cast<int>(strlen(m_szProgram));
        if (usearbvertexprograms != 0) {
            ASLgl::glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, nLen, m_szProgram);
            if (usearbvertexprograms != 0)
                AurGetVertexProgramARBError(m_szProgram);
        }

        if (part != 0 && currentscene != 0 && (flag & 1))
            currentscene->m_pLightManager->SetUpVertexProgramLightShaderConstants(m_nAllowedLights,
                                                                                 part->m_pGob == 0);

        m_bProgramLoaded = 1;
        return;
    }

BIND:
    if (usearbvertexprograms != 0)
        ASLgl::glBindProgramARB(GL_VERTEX_PROGRAM_ARB, *pProgramARB);
}

void AurVertexProgramMakeCurrent()
{
    currentframe++;
}
