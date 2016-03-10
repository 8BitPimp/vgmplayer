/*
 * http://www.musicdsp.org/showArchiveComment.php?ArchiveID=231
 *
 * Posted by Paul Sernine
 * 
 * These are /2 decimators, just instanciate one of them and use the Calc
 * method to obtain one sample while inputing two. There is 5,7 and 9 tap
 * versions.  They are extracted/adapted from a tutorial code by Thierry
 * Rochebois. The optimal coefficients are excerpts of Traitement numérique
 * du signal, 5eme edition, M Bellanger, Masson pp. 339-346.
 */

#pragma once

class decimate_5_t
{
private:
    float R1, R2, R3, R4, R5;

    const float h0;
    const float h1;
    const float h3;
    const float h5;
    
public:
    decimate_5_t()
        : h0( 346/692.0f)
        , h1( 208/692.0f)
        , h3(-44 /692.0f)
        , h5( 9  /692.0f)
    {
        R1=R2=R3=R4=R5=0.0f;
    }

    float operator () (const float x0, const float x1)
    {
        const float h5x0 = h5 * x0;
        const float h3x0 = h3 * x0;
        const float h1x0 = h1 * x0;
        
        const float R6   = R5 + h5x0;
        
        R5 = R4 + h3x0;
        R4 = R3 + h1x0;
        R3 = R2 + h1x0 + h0 * x1;
        R2 = R1 + h3x0;
        R1 = h5x0;
        
        return R6;
    }
};

class decimate_7_t
{
private:
    float R1, R2, R3, R4, R5, R6, R7;

    const float h0;
    const float h1;
    const float h3;
    const float h5;
    const float h7;
    
public:
    decimate_7_t()
        : h0( 802/1604.0f)
        , h1( 490/1604.0f)
        , h3(-116/1604.0f)
        , h5( 33 /1604.0f)
        , h7(-6  /1604.0f)
    {
        R1=R2=R3=R4=R5=R6=R7=0.0f;
    }
    
    float operator () (const float x0, const float x1)
    {
        const float h7x0 = h7 * x0;
        const float h5x0 = h5 * x0;
        const float h3x0 = h3 * x0;
        const float h1x0 = h1 * x0;
        
        const float R8 = R7 + h7x0;
        
        R7 = R6 + h5x0;
        R6 = R5 + h3x0;
        R5 = R4 + h1x0;
        R4 = R3 + h1x0 + h0 * x1;
        R3 = R2 + h3x0;
        R2 = R1 + h5x0;
        R1 = h7x0;
        
        return R8;
    }
};

class decimate_9_t
{
private:
    float R1, R2, R3, R4, R5, R6, R7, R8, R9;

    const float h0;
    const float h1;
    const float h3;
    const float h5;
    const float h7;
    const float h9;
    
public:

    decimate_9_t()
        : h0( 8192/16384.0f)
        , h1( 5042/16384.0f)
        , h3(-1277/16384.0f)
        , h5( 429 /16384.0f)
        , h7(-116 /16384.0f)
        , h9( 18  /16384.0f)
    {
        R1=R2=R3=R4=R5=R6=R7=R8=R9=0.0f;
    }
    
    float operator () (const float x0, const float x1)
    {
        const float h9x0 = h9 * x0;
        const float h7x0 = h7 * x0;
        const float h5x0 = h5 * x0;
        const float h3x0 = h3 * x0;
        const float h1x0 = h1 * x0;
        
        const float R10  = R9 + h9x0;
        
        R9 = R8 + h7x0;
        R8 = R7 + h5x0;
        R7 = R6 + h3x0;
        R6 = R5 + h1x0;
        R5 = R4 + h1x0 + h0 * x1;
        R4 = R3 + h3x0;
        R3 = R2 + h5x0;
        R2 = R1 + h7x0;
        R1 = h9x0;

        return R10;
    }
};
