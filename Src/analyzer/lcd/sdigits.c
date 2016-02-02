/*============================================================================*/
/* Font name: sdigits                                                         */
/* Characters: 224                                                            */
/* Maximal character x-size: 3                                                */
/* Characters y-size: 5                                                       */
/*============================================================================*/

#include "sdigits.h"

const uint8_t sdigits_height = 5;
const uint8_t sdigits_spacing = 1;
/*===================================*/
/*========= Characters data =========*/
/*===================================*/

/**  0x20 - 32  - ' '  **/
TCDATA sdigits_ssp[6]={3,
    b2b(0,0,0,0,0,0,0,0),
    b2b(0,0,0,0,0,0,0,0),
    b2b(0,0,0,0,0,0,0,0),
    b2b(0,0,0,0,0,0,0,0),
    b2b(0,0,0,0,0,0,0,0),
};

/**  0x2B - 43  - '+'  **/
TCDATA sdigits_spls[6]={3,
    b2b(0,0,0,0,0,0,0,0),
    b2b(0,0,0,0,0,0,1,0),
    b2b(0,0,0,0,0,1,1,1),
    b2b(0,0,0,0,0,0,1,0),
    b2b(0,0,0,0,0,0,0,0)};

/**  0x2D - 45  - '-'  **/
TCDATA sdigits_smin[6]={3,
    b2b(0,0,0,0,0,0,0,0),
    b2b(0,0,0,0,0,0,0,0),
    b2b(0,0,0,0,0,1,1,1),
    b2b(0,0,0,0,0,0,0,0),
    b2b(0,0,0,0,0,0,0,0)};

/**  0x2E - 46  - '.'  **/
TCDATA sdigits_sssp[6]={1,
    b2b(0,0,0,0,0,0,0,0),
    b2b(0,0,0,0,0,0,0,0),
    b2b(0,0,0,0,0,0,0,0),
    b2b(0,0,0,0,0,0,0,0),
    b2b(0,0,0,0,0,0,0,1)};

/**  0x30 - 48  - '0'  **/
TCDATA sdigits_n0[6]={3,
    b2b(0,0,0,0,0,1,1,1),
    b2b(0,0,0,0,0,1,0,1),
    b2b(0,0,0,0,0,1,0,1),
    b2b(0,0,0,0,0,1,0,1),
    b2b(0,0,0,0,0,1,1,1)};

/**  0x31 - 49  - '1'  **/
TCDATA sdigits_n1[6]={3,
    b2b(0,0,0,0,0,0,0,1),
    b2b(0,0,0,0,0,0,1,1),
    b2b(0,0,0,0,0,0,0,1),
    b2b(0,0,0,0,0,0,0,1),
    b2b(0,0,0,0,0,0,0,1)};

/**  0x32 - 50  - '2'  **/
TCDATA sdigits_n2[6]={3,
    b2b(0,0,0,0,0,1,1,1),
    b2b(0,0,0,0,0,0,0,1),
    b2b(0,0,0,0,0,1,1,1),
    b2b(0,0,0,0,0,1,0,0),
    b2b(0,0,0,0,0,1,1,1)};

/**  0x33 - 51  - '3'  **/
TCDATA sdigits_n3[6]={3,
    b2b(0,0,0,0,0,1,1,1),
    b2b(0,0,0,0,0,0,0,1),
    b2b(0,0,0,0,0,1,1,1),
    b2b(0,0,0,0,0,0,0,1),
    b2b(0,0,0,0,0,1,1,1)};

/**  0x34 - 52  - '4'  **/
TCDATA sdigits_n4[6]={3,
    b2b(0,0,0,0,0,1,0,1),
    b2b(0,0,0,0,0,1,0,1),
    b2b(0,0,0,0,0,1,1,1),
    b2b(0,0,0,0,0,0,0,1),
    b2b(0,0,0,0,0,0,0,1)};

/**  0x35 - 53  - '5'  **/
TCDATA sdigits_n5[6]={3,
    b2b(0,0,0,0,0,1,1,1),
    b2b(0,0,0,0,0,1,0,0),
    b2b(0,0,0,0,0,1,1,1),
    b2b(0,0,0,0,0,0,0,1),
    b2b(0,0,0,0,0,1,1,1)};

/**  0x36 - 54  - '6'  **/
TCDATA sdigits_n6[6]={3,
    b2b(0,0,0,0,0,1,1,1),
    b2b(0,0,0,0,0,1,0,0),
    b2b(0,0,0,0,0,1,1,1),
    b2b(0,0,0,0,0,1,0,1),
    b2b(0,0,0,0,0,1,1,1)};

/**  0x37 - 55  - '7'  **/
TCDATA sdigits_n7[6]={3,
    b2b(0,0,0,0,0,1,1,1),
    b2b(0,0,0,0,0,0,0,1),
    b2b(0,0,0,0,0,0,1,0),
    b2b(0,0,0,0,0,0,1,0),
    b2b(0,0,0,0,0,0,1,0)};

/**  0x38 - 56  - '8'  **/
TCDATA sdigits_n8[6]={3,
    b2b(0,0,0,0,0,1,1,1),
    b2b(0,0,0,0,0,1,0,1),
    b2b(0,0,0,0,0,1,1,1),
    b2b(0,0,0,0,0,1,0,1),
    b2b(0,0,0,0,0,1,1,1)};

/**  0x39 - 57  - '9'  **/
TCDATA sdigits_n9[6]={3,
    b2b(0,0,0,0,0,1,1,1),
    b2b(0,0,0,0,0,1,0,1),
    b2b(0,0,0,0,0,1,1,1),
    b2b(0,0,0,0,0,0,0,1),
    b2b(0,0,0,0,0,1,1,1)};


/**  0x41 - 65  - 'A'  **/
TCDATA sdigits_UA[6]={3,
    b2b(0,0,0,0,0,1,1,1),
    b2b(0,0,0,0,0,1,0,1),
    b2b(0,0,0,0,0,1,1,1),
    b2b(0,0,0,0,0,1,0,1),
    b2b(0,0,0,0,0,1,0,1)};

/**  0x42 - 66  - 'B'  **/
TCDATA sdigits_UB[6]={3,
    b2b(0,0,0,0,0,1,1,0),
    b2b(0,0,0,0,0,1,0,1),
    b2b(0,0,0,0,0,1,1,0),
    b2b(0,0,0,0,0,1,0,1),
    b2b(0,0,0,0,0,1,1,0)};

/**  0x43 - 67  - 'C'  **/
TCDATA sdigits_UC[6]={3,
    b2b(0,0,0,0,0,1,1,1),
    b2b(0,0,0,0,0,1,0,0),
    b2b(0,0,0,0,0,1,0,0),
    b2b(0,0,0,0,0,1,0,0),
    b2b(0,0,0,0,0,1,1,1)};

/**  0x44 - 68  - 'D'  **/
TCDATA sdigits_UD[6]={3,
    b2b(0,0,0,0,0,1,1,0),
    b2b(0,0,0,0,0,1,0,1),
    b2b(0,0,0,0,0,1,0,1),
    b2b(0,0,0,0,0,1,0,1),
    b2b(0,0,0,0,0,1,1,0)};

/**  0x45 - 69  - 'E'  **/
TCDATA sdigits_UE[6]={3,
    b2b(0,0,0,0,0,1,1,1),
    b2b(0,0,0,0,0,1,0,0),
    b2b(0,0,0,0,0,1,1,1),
    b2b(0,0,0,0,0,1,0,0),
    b2b(0,0,0,0,0,1,1,1)};

/**  0x46 - 70  - 'F'  **/
TCDATA sdigits_UF[6]={3,
    b2b(0,0,0,0,0,1,1,1),
    b2b(0,0,0,0,0,1,0,0),
    b2b(0,0,0,0,0,1,1,1),
    b2b(0,0,0,0,0,1,0,0),
    b2b(0,0,0,0,0,1,0,0)};

/*  'L'  */
TCDATA sdigits_UL[6]={3,
    b2b(0,0,0,0,0,1,0,0),
    b2b(0,0,0,0,0,1,0,0),
    b2b(0,0,0,0,0,1,0,0),
    b2b(0,0,0,0,0,1,0,0),
    b2b(0,0,0,0,0,1,1,1)};

/*  'Z'  **/
TCDATA sdigits_UZ[6]={3,
    b2b(0,0,0,0,0,1,1,1),
    b2b(0,0,0,0,0,0,0,1),
    b2b(0,0,0,0,0,0,1,0),
    b2b(0,0,0,0,0,1,0,0),
    b2b(0,0,0,0,0,1,1,1)};

/**  0x100 - 256  **/
TCDATA sdigits_blank[6]={3,
    b2b(0,0,0,0,0,0,0,0),
    b2b(0,0,0,0,0,0,0,0),
    b2b(0,0,0,0,0,0,0,0),
    b2b(0,0,0,0,0,0,0,0),
    b2b(0,0,0,0,0,0,0,0)
};

/*============================================*/
/*========= Character pointers table =========*/
/*============================================*/

const uint8_t* const sdigits[256] =
        {sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_blank,
         sdigits_ssp,
         sdigits_blank, //sdigits_sexc,
         sdigits_blank, //sdigits_sdq,
         sdigits_blank, //sdigits_sfr,
         sdigits_blank, //sdigits_sdlr,
         sdigits_blank, //sdigits_sprc,
         sdigits_blank, //sdigits_sand,
         sdigits_blank, //sdigits_sap,
         sdigits_blank, //sdigits_sprl,
         sdigits_blank, //sdigits_sprr,
         sdigits_blank, //sdigits_sstr,
         sdigits_spls,
         sdigits_blank, //sdigits_scmm,
         sdigits_smin,
         sdigits_sssp,
         sdigits_blank, //sdigits_sslh,
         sdigits_n0,
         sdigits_n1,
         sdigits_n2,
         sdigits_n3,
         sdigits_n4,
         sdigits_n5,
         sdigits_n6,
         sdigits_n7,
         sdigits_n8,
         sdigits_n9,
         sdigits_blank, //sdigits_scln,
         sdigits_blank, //sdigits_ssmc,
         sdigits_blank, //sdigits_sbrl,
         sdigits_blank, //sdigits_seql,
         sdigits_blank, //sdigits_sbrr,
         sdigits_blank, //sdigits_sqst,
         sdigits_blank, //sdigits_szvn,
         sdigits_UA,
         sdigits_UB,
         sdigits_UC,
         sdigits_UD,
         sdigits_UE,
         sdigits_UF,
         sdigits_blank, //sdigits_UG,
         sdigits_blank, //sdigits_UH,
         sdigits_blank, //sdigits_UI,
         sdigits_blank, //sdigits_UJ,
         sdigits_blank, //sdigits_UK,
         sdigits_UL,
         sdigits_blank, //sdigits_UM,
         sdigits_blank, //sdigits_UN,
         sdigits_blank, //sdigits_UO,
         sdigits_blank, //sdigits_UP,
         sdigits_blank, //sdigits_UQ,
         sdigits_blank, //sdigits_UR,
         sdigits_blank, //sdigits_US,
         sdigits_blank, //sdigits_UT,
         sdigits_blank, //sdigits_UU,
         sdigits_blank, //sdigits_UV,
         sdigits_blank, //sdigits_UW,
         sdigits_blank, //sdigits_UX,
         sdigits_blank, //sdigits_UY,
         sdigits_UZ,
         sdigits_blank, //sdigits_sbl,
         sdigits_blank, //sdigits_sbsl,
         sdigits_blank, //sdigits_sbr,
         sdigits_blank, //sdigits_ssqr,
         sdigits_blank, //sdigits_sul,
         sdigits_blank, //sdigits_shc,
         sdigits_UA, //sdigits_la,
         sdigits_UB, //sdigits_lb,
         sdigits_UC, //sdigits_lc,
         sdigits_UD, //sdigits_ld,
         sdigits_UE, //sdigits_le,
         sdigits_UF, //sdigits_lf,
         sdigits_blank, //sdigits_lg,
         sdigits_blank, //sdigits_lh,
         sdigits_blank, //sdigits_li,
         sdigits_blank, //sdigits_lj,
         sdigits_blank, //sdigits_lk,
         sdigits_blank, //sdigits_ll,
         sdigits_blank, //sdigits_lm,
         sdigits_blank, //sdigits_ln,
         sdigits_blank, //sdigits_lo,
         sdigits_blank, //sdigits_lp,
         sdigits_blank, //sdigits_lq,
         sdigits_blank, //sdigits_lr,
         sdigits_blank, //sdigits_ls,
         sdigits_blank, //sdigits_lt,
         sdigits_blank, //sdigits_lu,
         sdigits_blank, //sdigits_lv,
         sdigits_blank, //sdigits_lw,
         sdigits_blank, //sdigits_lx,
         sdigits_blank, //sdigits_ly,
         sdigits_blank, //sdigits_lz,
         sdigits_blank, //sdigits_spsl,
         sdigits_blank, //sdigits_svli,
         sdigits_blank, //sdigits_spsh,
         sdigits_blank, //sdigits_svwe,
         sdigits_blank, //sdigits_c127,
         sdigits_blank, //sdigits_c128,
         sdigits_blank, //sdigits_c129,
         sdigits_blank, //sdigits_c130,
         sdigits_blank, //sdigits_c131,
         sdigits_blank, //sdigits_c132,
         sdigits_blank, //sdigits_c133,
         sdigits_blank, //sdigits_c134,
         sdigits_blank, //sdigits_c135,
         sdigits_blank, //sdigits_c136,
         sdigits_blank, //sdigits_c137,
         sdigits_blank, //sdigits_USh,
         sdigits_blank, //sdigits_c139,
         sdigits_blank, //sdigits_c140,
         sdigits_blank, //sdigits_UTh,
         sdigits_blank, //sdigits_UZh,
         sdigits_blank, //sdigits_c143,
         sdigits_blank, //sdigits_c144,
         sdigits_blank, //sdigits_c145,
         sdigits_blank, //sdigits_c146,
         sdigits_blank, //sdigits_c147,
         sdigits_blank, //sdigits_c148,
         sdigits_blank, //sdigits_c149,
         sdigits_blank, //sdigits_c150,
         sdigits_blank, //sdigits_c151,
         sdigits_blank, //sdigits_c152,
         sdigits_blank, //sdigits_c153,
         sdigits_blank, //sdigits_lsh,
         sdigits_blank, //sdigits_c155,
         sdigits_blank, //sdigits_c156,
         sdigits_blank, //sdigits_lth,
         sdigits_blank, //sdigits_lzh,
         sdigits_blank, //sdigits_c159,
         sdigits_blank, //sdigits_sst,
         sdigits_blank, //sdigits_ssh,
         sdigits_blank, //sdigits_c162,
         sdigits_blank, //sdigits_c163,
         sdigits_blank, //sdigits_c164,
         sdigits_blank, //sdigits_c165,
         sdigits_blank, //sdigits_c166,
         sdigits_blank, //sdigits_c167,
         sdigits_blank, //sdigits_c168,
         sdigits_blank, //sdigits_c169,
         sdigits_blank, //sdigits_c170,
         sdigits_blank, //sdigits_c171,
         sdigits_blank, //sdigits_c172,
         sdigits_blank, //sdigits_c173,
         sdigits_blank, //sdigits_c174,
         sdigits_blank, //sdigits_c175,
         sdigits_blank, //sdigits_ssk,
         sdigits_blank, //sdigits_c177,
         sdigits_blank, //sdigits_c178,
         sdigits_blank, //sdigits_c179,
         sdigits_blank, //sdigits_ssc,
         sdigits_blank, //sdigits_c181,
         sdigits_blank, //sdigits_c182,
         sdigits_blank, //sdigits_c183,
         sdigits_blank, //sdigits_c184,
         sdigits_blank, //sdigits_c185,
         sdigits_blank, //sdigits_c186,
         sdigits_blank, //sdigits_c187,
         sdigits_blank, //sdigits_c188,
         sdigits_blank, //sdigits_c189,
         sdigits_blank, //sdigits_c190,
         sdigits_blank, //sdigits_c191,
         sdigits_blank, //sdigits_c192,
         sdigits_blank, //sdigits_UAc,
         sdigits_blank, //sdigits_c194,
         sdigits_blank, //sdigits_c195,
         sdigits_blank, //sdigits_c196,
         sdigits_blank, //sdigits_c197,
         sdigits_blank, //sdigits_c198,
         sdigits_blank, //sdigits_c199,
         sdigits_blank, //sdigits_UCh,
         sdigits_blank, //sdigits_UEc,
         sdigits_blank, //sdigits_c202,
         sdigits_blank, //sdigits_c203,
         sdigits_blank, //sdigits_UEh,
         sdigits_blank, //sdigits_UIc,
         sdigits_blank, //sdigits_c206,
         sdigits_blank, //sdigits_UDh,
         sdigits_blank, //sdigits_c208,
         sdigits_blank, //sdigits_c209,
         sdigits_blank, //sdigits_UMh,
         sdigits_blank, //sdigits_UOc,
         sdigits_blank, //sdigits_c212,
         sdigits_blank, //sdigits_c213,
         sdigits_blank, //sdigits_c214,
         sdigits_blank, //sdigits_c215,
         sdigits_blank, //sdigits_URh,
         sdigits_blank, //sdigits_UUk,
         sdigits_blank, //sdigits_UUc,
         sdigits_blank, //sdigits_c219,
         sdigits_blank, //sdigits_c220,
         sdigits_blank, //sdigits_UYc,
         sdigits_blank, //sdigits_c222,
         sdigits_blank, //sdigits_c223,
         sdigits_blank, //sdigits_c224,
         sdigits_blank, //sdigits_lac,
         sdigits_blank, //sdigits_c226,
         sdigits_blank, //sdigits_c227,
         sdigits_blank, //sdigits_c228,
         sdigits_blank, //sdigits_c229,
         sdigits_blank, //sdigits_c230,
         sdigits_blank, //sdigits_c231,
         sdigits_blank, //sdigits_lch,
         sdigits_blank, //sdigits_lec,
         sdigits_blank, //sdigits_c234,
         sdigits_blank, //sdigits_c235,
         sdigits_blank, //sdigits_leh,
         sdigits_blank, //sdigits_lic,
         sdigits_blank, //sdigits_c238,
         sdigits_blank, //sdigits_ldh,
         sdigits_blank, //sdigits_c240,
         sdigits_blank, //sdigits_c241,
         sdigits_blank, //sdigits_lnh,
         sdigits_blank, //sdigits_loc,
         sdigits_blank, //sdigits_c244,
         sdigits_blank, //sdigits_c245,
         sdigits_blank, //sdigits_c246,
         sdigits_blank, //sdigits_c247,
         sdigits_blank, //sdigits_lrh,
         sdigits_blank, //sdigits_luk,
         sdigits_blank, //sdigits_luc,
         sdigits_blank, //sdigits_c251,
         sdigits_blank, //sdigits_c252,
         sdigits_blank, //sdigits_lyc,
         sdigits_blank, //sdigits_c254,
         sdigits_blank  //sdigits_c255
};
