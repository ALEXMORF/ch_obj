#pragma once

/*

NOTE(chen):

dependencies:

. ch_buf.h

 to use this library, include the file like this:
 
#define CH_OBJ_IMPLEMENTATION
#include "ch_obj.h"

*/

/*

TODO(chen):

. triangulate quad faces
. parse different material encoding
. add parallelism for faster speed

*/

//
//
// API

struct material
{
    char Name[255];
    float Albedo[4];
    float Emission[3];
};

struct vertex
{
    float P[3];
    float N[3];
    float Albedo[3];
    float Emission[3];
};

vertex *LoadObj(char *Path);

//
//
// Implementation

#ifdef CH_OBJ_IMPLEMENTATION

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include "ch_buf.h"

#define ASSERT(Value) assert(Value)

static char *
ReadFile(char *FilePath)
{
    char *Buffer = 0;
    
    FILE *File = fopen(FilePath, "rb");
    if (File)
    {
        fseek(File, 0, SEEK_END);
        int FileSize = ftell(File);
        rewind(File);
        
        Buffer = (char *)malloc(FileSize+1);
        Buffer[FileSize] = 0;
        fread(Buffer, 1, FileSize, File);
        
        fclose(File);
    }
    
    return Buffer;
}

inline char *
GotoNextLine(char *Ptr)
{
    while (*Ptr != '\n')
    {
        Ptr += 1;
    }
    
    while (*Ptr == '\r' || *Ptr == '\n')
    {
        Ptr += 1;
    }
    
    return Ptr;
}

inline int
Length(char *Str)
{
    int Count = 0;
    
    while (*Str)
    {
        Count += 1;
        Str += 1;
    }
    
    return Count;
}

inline bool
Equal(char *Str1, char *Str2)
{
    while (*Str1 && *Str2)
    {
        if (*Str1++ != *Str2++)
        {
            return false;
        }
    }
    return true;
}

inline bool
StartsWith(char *Str, char *SubStr)
{
    for (int CharIndex = 0; CharIndex < Length(SubStr); ++CharIndex)
    {
        if (Str[CharIndex] != SubStr[CharIndex])
        {
            return false;
        }
    }
    
    return true;
}

inline char *
SkipSpaces(char *Cursor)
{
    char *EndCursor = Cursor;
    
    while (*EndCursor == ' ')
    {
        EndCursor += 1;
    }
    
    return EndCursor;
}

#define EXPECT_STR(Cursor, S) \
if (!StartsWith(Cursor, S)) \
{ \
    return false; \
} \
Cursor += Length(S); \

#define EXPECT_CHAR(Cursor, C) \
if (*Cursor != C) \
{ \
    return false; \
} \
Cursor += 1; \

inline bool
ParseString(char *Cursor, char *Prefix, char *Buffer_Out)
{
    Cursor = SkipSpaces(Cursor);
    
    EXPECT_STR(Cursor, Prefix);
    
    Cursor = SkipSpaces(Cursor);
    
    int BufferIndex = 0;
    while (*Cursor != '\n')
    {
        Buffer_Out[BufferIndex++] = *Cursor++;
    }
    
    return true;
}

inline char *
ReadInteger(char *Str, int32_t *Integer_Out)
{
    char *Cursor = Str;
    Cursor = SkipSpaces(Cursor);
    
    int Sign = 1;
    if (*Cursor == '-')
    {
        Sign = -1;
        Cursor += 1;
    }
    
    int Integer = 0;
    while (*Cursor >= '0' && *Cursor <= '9')
    {
        int Digit = *Cursor++ - '0';
        Integer = Integer * 10 + Digit;
    }
    
    *Integer_Out = Sign * Integer;
    return Cursor;
}

inline char *
ReadFloat(char *Str, float *Float_Out)
{
    char *Cursor = Str;
    Cursor = SkipSpaces(Cursor);
    
    float Sign = 1.0f;
    if (*Cursor == '-')
    {
        Sign = -1.0f;
        Cursor += 1;
    }
    
    int WholeNum = 0;
    float DeciNum = 0.0f;
    
    while (*Cursor >= '0' && *Cursor <= '9')
    {
        int Digit = *Cursor++ - '0';
        WholeNum = 10 * WholeNum + Digit;
    }
    
    if (*Cursor == '.')
    {
        Cursor += 1;
        
        float Weight = 0.1f;
        while (*Cursor >= '0' && *Cursor <= '9')
        {
            int Digit = *Cursor++ - '0';
            DeciNum += Weight * (float)Digit;
            Weight *= 0.1f;
        }
    }
    
    //NOTE(chen): scientific notation
    float E = 1.0f;
    if (*Cursor == 'e' || *Cursor == 'E')
    {
        Cursor += 1;
        
        int Exponent = 0;
        Cursor = ReadInteger(Cursor, &Exponent);
        E = powf(10.0f, (float)Exponent);
    }
    
    *Float_Out = Sign * ((float)WholeNum + DeciNum) * E;
    return Cursor;
}

inline bool
ParseFloat(char *Cursor, char *Prefix, float *Float_Out)
{
    Cursor = SkipSpaces(Cursor);
    EXPECT_STR(Cursor, Prefix);
    Cursor = SkipSpaces(Cursor);
    
    Cursor = ReadFloat(Cursor, Float_Out);
    
    return true;
}

inline bool
ParseV3(char *Cursor, char *Prefix, float V_Out[3])
{
    Cursor = SkipSpaces(Cursor);
    EXPECT_STR(Cursor, Prefix);
    Cursor = SkipSpaces(Cursor);
    
    for (int I = 0; I < 3; ++I)
    {
        Cursor = ReadFloat(Cursor, V_Out + I);
    }
    
    return true;
}

inline bool 
ParseFaceWithTexCoord(char *Cursor, int *VertexIndices, int *NormalIndices)
{
    Cursor = SkipSpaces(Cursor);
    EXPECT_CHAR(Cursor, 'f');
    
    for (int PointIndex = 0; PointIndex < 3; ++PointIndex)
    {
        Cursor = SkipSpaces(Cursor);
        
        int VertexIndex, TexIndex, NormalIndex;
        Cursor = ReadInteger(Cursor, &VertexIndex);
        EXPECT_CHAR(Cursor, '/');
        Cursor = ReadInteger(Cursor, &TexIndex);
        EXPECT_CHAR(Cursor, '/');
        Cursor = ReadInteger(Cursor, &NormalIndex);
        
        VertexIndices[PointIndex] = VertexIndex;
        NormalIndices[PointIndex] = NormalIndex;
    }
    
    return true;
}

#undef EXPECT_STR
#undef EXPECT_CHAR

static vertex *
LoadObj(char *Path)
{
    vertex *Vertices = {};
    
    char MtlPath[255];
    snprintf(MtlPath, sizeof(MtlPath), "%s.mtl", Path);
    char ObjPath[255];
    snprintf(ObjPath, sizeof(ObjPath), "%s.obj", Path);
    
    material *Mats = 0;
    
    char *MtlFileContent = ReadFile(MtlPath);
    if (MtlFileContent)
    {
        char *MtlFileWalker = MtlFileContent;
        
        while (*MtlFileWalker)
        {
            if (StartsWith(MtlFileWalker, "newmtl"))
            {
                material NewMat = {};
                ParseString(MtlFileWalker, "newmtl", NewMat.Name);
                BufPush(Mats, NewMat);
            }
            else if (StartsWith(MtlFileWalker, "Kd"))
            {
                ASSERT(BufCount(Mats) > 0);
                
                float Albedo3[3] = {};
                ParseV3(MtlFileWalker, "Kd", Albedo3);
                BufLast(Mats).Albedo[0] = Albedo3[0];
                BufLast(Mats).Albedo[1] = Albedo3[1];
                BufLast(Mats).Albedo[2] = Albedo3[2];
                BufLast(Mats).Albedo[3] = 1.0f;
            }
            else if (StartsWith(MtlFileWalker, "d"))
            {
                ASSERT(BufCount(Mats) > 0);
                ParseFloat(MtlFileWalker, "d", &BufLast(Mats).Albedo[3]);
            }
            else if (StartsWith(MtlFileWalker, "Ke"))
            {
                ASSERT(BufCount(Mats) > 0);
                ParseV3(MtlFileWalker, "Ke", BufLast(Mats).Emission);
            }
            
            MtlFileWalker = GotoNextLine(MtlFileWalker);
        }
    }
    free(MtlFileContent);
    
    char *ObjFileContent = ReadFile(ObjPath);
    if (!ObjFileContent)
    {
        return 0;
    }
    char *ObjFileWalker = ObjFileContent;
    
    float *TempVertices = 0;
    float *TempNormals = 0;
    
    Vertices = 0;
    int AlphaVertexCount = 0;
    
    int CurrentMatIndex = -1;
    ObjFileWalker = ObjFileContent;
    while (*ObjFileWalker)
    {
        if (StartsWith(ObjFileWalker, "vn"))
        {
            float Normal[3] = {};
            ParseV3(ObjFileWalker, "vn", Normal);
            
            BufPush(TempNormals, Normal[0]);
            BufPush(TempNormals, Normal[1]);
            BufPush(TempNormals, Normal[2]);
        }
        else if (StartsWith(ObjFileWalker, "v"))
        {
            float Vertex[3] = {};
            ParseV3(ObjFileWalker, "v", Vertex);
            
            BufPush(TempVertices, Vertex[0]);
            BufPush(TempVertices, Vertex[1]);
            BufPush(TempVertices, Vertex[2]);
        }
        else if (StartsWith(ObjFileWalker, "f"))
        {
            //NOTE(chen): discards faces with alpha less than 1
            bool FaceIsTransparent = (CurrentMatIndex != -1 &&
                                      Mats[CurrentMatIndex].Albedo[3] < 0.9f);
            
            if (!FaceIsTransparent)
            {
                int VertexIndices[3] = {};
                int NormalIndices[3] = {};
                
                bool ParsedFace = ParseFaceWithTexCoord(ObjFileWalker, VertexIndices, NormalIndices);
                ASSERT(ParsedFace);
                
                for (int VI = 0; VI < 3; ++VI)
                {
                    //NOTE(chen): negative index wraps
                    if (VertexIndices[VI] < 0)
                    {
                        VertexIndices[VI] += (int)BufCount(TempVertices);
                    }
                    else
                    {
                        //NOTE(chen): OBJ is 1-based, making it 0-based
                        VertexIndices[VI] -= 1;
                    }
                    
                    //NOTE(chen): negative index wraps
                    if (NormalIndices[VI] < 0)
                    {
                        NormalIndices[VI] += (int)BufCount(TempNormals);
                    }
                    else
                    {
                        //NOTE(chen): OBJ is 1-based, making it 0-based
                        NormalIndices[VI] -= 1;
                    }
                    
                    ASSERT(VertexIndices[VI] >= 0 && VertexIndices[VI] < BufCount(TempVertices));
                    ASSERT(NormalIndices[VI] >= 0 && NormalIndices[VI] < BufCount(TempNormals));
                    
                    vertex NewVertex = {};
                    
                    int VertexStartIndex = VertexIndices[VI] * 3;
                    memcpy(NewVertex.P, TempVertices + VertexStartIndex,
                           sizeof(float) * 3);
                    
                    int NormalStartIndex = NormalIndices[VI] * 3;
                    memcpy(NewVertex.N, TempNormals + NormalStartIndex,
                           sizeof(float) * 3);
                    
                    if (CurrentMatIndex != -1)
                    {
                        memcpy(NewVertex.Albedo, Mats[CurrentMatIndex].Albedo,
                               sizeof(float) * 3);
                        memcpy(NewVertex.Emission, Mats[CurrentMatIndex].Emission,
                               sizeof(float) * 3);
                    }
                    else
                    {
                        //NOTE(chen): default material
                        NewVertex.Albedo[0] = 0.64f;
                        NewVertex.Albedo[1] = 0.64f;
                        NewVertex.Albedo[2] = 0.64f;
                        
                        NewVertex.Emission[0] = 0.0f;
                        NewVertex.Emission[1] = 0.0f;
                        NewVertex.Emission[2] = 0.0f;
                    }
                    
                    BufPush(Vertices, NewVertex);
                }
            }
            else
            {
                AlphaVertexCount += 3;
            }
        }
        else if (StartsWith(ObjFileWalker, "usemtl"))
        {
            char MtlName[255];
            ParseString(ObjFileWalker, "usemtl", MtlName);
            
            int NewMatIndex = -1;
            for (int MatIndex = 0; MatIndex < BufCount(Mats); ++MatIndex)
            {
                if (Equal(MtlName, Mats[MatIndex].Name))
                {
                    NewMatIndex = MatIndex;
                    break;
                }
            }
            
            ASSERT(NewMatIndex != -1);
            CurrentMatIndex = NewMatIndex;
        }
        
        ObjFileWalker = GotoNextLine(ObjFileWalker);
    }
    free(ObjFileContent);
    
    BufFree(Mats);
    BufFree(TempVertices);
    BufFree(TempNormals);
    return Vertices;
}

#endif