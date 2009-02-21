/*

-Header_File SpiceEK.h ( CSPICE EK-specific definitions )

-Abstract

   Perform CSPICE EK-specific definitions, including macros and user-
   defined types.
         
-Disclaimer

   THIS SOFTWARE AND ANY RELATED MATERIALS WERE CREATED BY THE
   CALIFORNIA INSTITUTE OF TECHNOLOGY (CALTECH) UNDER A U.S.
   GOVERNMENT CONTRACT WITH THE NATIONAL AERONAUTICS AND SPACE
   ADMINISTRATION (NASA). THE SOFTWARE IS TECHNOLOGY AND SOFTWARE
   PUBLICLY AVAILABLE UNDER U.S. EXPORT LAWS AND IS PROVIDED "AS-IS"
   TO THE RECIPIENT WITHOUT WARRANTY OF ANY KIND, INCLUDING ANY
   WARRANTIES OF PERFORMANCE OR MERCHANTABILITY OR FITNESS FOR A
   PARTICULAR USE OR PURPOSE (AS SET FORTH IN UNITED STATES UCC
   SECTIONS 2312-2313) OR FOR ANY PURPOSE WHATSOEVER, FOR THE
   SOFTWARE AND RELATED MATERIALS, HOWEVER USED.

   IN NO EVENT SHALL CALTECH, ITS JET PROPULSION LABORATORY, OR NASA
   BE LIABLE FOR ANY DAMAGES AND/OR COSTS, INCLUDING, BUT NOT
   LIMITED TO, INCIDENTAL OR CONSEQUENTIAL DAMAGES OF ANY KIND,
   INCLUDING ECONOMIC DAMAGE OR INJURY TO PROPERTY AND LOST PROFITS,
   REGARDLESS OF WHETHER CALTECH, JPL, OR NASA BE ADVISED, HAVE
   REASON TO KNOW, OR, IN FACT, SHALL KNOW OF THE POSSIBILITY.

   RECIPIENT BEARS ALL RISK RELATING TO QUALITY AND PERFORMANCE OF
   THE SOFTWARE AND ANY RELATED MATERIALS, AND AGREES TO INDEMNIFY
   CALTECH AND NASA FOR ALL THIRD-PARTY CLAIMS RESULTING FROM THE
   ACTIONS OF RECIPIENT IN THE USE OF THE SOFTWARE.

-Required_Reading

   None.
   
-Particulars

   This header defines macros, enumerated types, structures, and 
   typedefs that may be referenced in application code that calls CSPICE
   EK functions.
   

   Macros
   ======
   
      General limits
      --------------
      
         Name                  Description
         ----                  ----------
         SPICE_EK_MXCLSG       Maximum number of columns per segment.
         
         SPICE_EK_TYPLEN       Maximum length of a short string 
                               indicating a data type (one of 
                               {"CHR", "DP", "INT", "TIME"}). Such 
                               strings are returned by some of the 
                               Fortran SPICELIB EK routines, hence also
                               by their f2c'd counterparts.
   
      Sizes of EK objects
      -------------------
      
         Name                  Description
         ----                  ----------
         
         SPICE_EK_CNAMSZ       Maximum length of column name.
         SPICE_EK_CSTRLN       Length of string required to hold column
                               name.  
         SPICE_EK_TNAMSZ       Maximum length of table name.
         SPICE_EK_TSTRLN       Length of string required to hold table
                               name.  
         
   
      Query-related limits
      --------------------
      
         Name                  Description
         ----                  ----------
         
         SPICE_EK_MAXQRY       Maximum length of an input query.  This 
                               value is currently equivalent to 
                               twenty-five 80-character lines.
         
         SPICE_EK_MAXQSEL      Maximum number of columns that may be
                               listed in the `SELECT clause' of a query.
         
         SPICE_EK_MAXQTAB      Maximum number of tables that may be 
                               listed in the `FROM clause' of a query.
         
         SPICE_EK_MAXQCON      Maximum number of relational expressions 
                               that may be listed in the `constraint 
                               clause' of a query.
    
                               This limit applies to a query when it is 
                               represented in `normalized form': that 
                               is, the constraints have been expressed 
                               as a disjunction of conjunctions of 
                               relational expressions. The number of 
                               relational expressions in a query that 
                               has been expanded in this fashion may be 
                               greater than the number of relations in 
                               the query as orginally written. For 
                               example, the expression
     
                                       ( ( A LT 1 ) OR ( B GT 2 ) )
                                  AND
                                       ( ( C NE 3 ) OR ( D EQ 4 ) )
                          
                               which contains 4 relational expressions, 
                               expands to the equivalent normalized 
                               constraint
                          
                                       (  ( A LT 1 ) AND ( C NE 3 )  )
                                  OR
                                       (  ( A LT 1 ) AND ( D EQ 4 )  )
                                  OR
                                       (  ( B GT 2 ) AND ( C NE 3 )  )
                                  OR
                                       (  ( B GT 2 ) AND ( D EQ 4 )  )
                          
                               which contains eight relational 
                               expressions.
                          
   
         
         SPICE_EK_MAXQJOIN     Maximum number of tables that can be 
                               joined.
         
         SPICE_EK_MAXQJCON     Maximum number of join constraints 
                               allowed.
         
         SPICE_EK_MAXQORD      Maximum number of columns that may be 
                               used in the `order-by clause' of a query.
         
         SPICE_EK_MAXQTOK      Maximum number of tokens in a query.  
                               Tokens
                               are reserved words, column names,   
                               parentheses, and values. Literal strings
                               and time values count as single tokens.
         
         SPICE_EK_MAXQNUM      Maximum number of numeric tokens in a 
                               query.
         
         SPICE_EK_MAXQCLN      Maximum total length of character tokens
                               in a query.
         
         SPICE_EK_MAXQSTR      Maximum length of literal string values 
                               allowed in queries.
         
   
      Codes
      -----
      
         Name                  Description
         ----                  ----------
         
         SPICE_EK_VARSIZ       Code used to indicate variable-size 
                               objects. Usually this is used in a 
                               context where a non-negative integer 
                               indicates the size of a fixed-size object
                               and the presence of this code indicates a
                               variable-size object.
      
                               The value of this constant must match the 
                               parameter IFALSE used in the Fortran 
                               library SPICELIB.


   Enumerated Types
   ================

      Enumerated code values
      ----------------------
      
         Name                  Description
         ----                  ----------
         SpiceEKDataType       Codes for data types used in the EK 
                               interface: character, double precision,
                               integer, and "time."
 
                               The values are:
                               
                                 { SPICE_CHR  = 0, 
                                   SPICE_DP   = 1, 
                                   SPICE_INT  = 2,
                                   SPICE_TIME = 3 }



         SpiceEKExprClass      Codes for types of expressions that may
                               appear in the SELECT clause of EK 
                               queries.  Values and meanings are:


                                  SPICE_EK_EXP_COL   Selected item was a 
                                                     column. The column 
                                                     may qualified by a
                                                     table name. 
 
                                  SPICE_EK_EXP_FUNC  Selected item was 
                                                     a simple function 
                                                     invocation of the 
                                                     form 
 
                                                        F ( <column> ) 
 
                                                     or else was 
                                                     
                                                        COUNT(*) 
 
                                  SPICE_EK_EXP_EXPR  Selected item was a
                                                     more general 
                                                     expression than 
                                                     those shown above. 
   
   
                               Numeric values are:
                               
                                 { SPICE_EK_EXP_COL  = 0, 
                                   SPICE_EK_EXP_FUNC = 1, 
                                   SPICE_EK_EXP_EXPR = 2 }
                               
                               
   Structures
   ==========
   
      EK API structures
      -----------------

         Name                  Description
         ----                  ----------
   
         SpiceEKAttDsc         EK column attribute descriptor.  Note 
                               that this object is distinct from the EK
                               column descriptors used internally in 
                               the EK routines; those descriptors 
                               contain pointers as well as attribute 
                               information.

                               The members are:
 
                                  cclass:     Column class code.

                                  dtype:      Data type code:  has type
                                              SpiceEKDataType.
                                              
                                  strlen:     String length.  Applies to 
                                              SPICE_CHR type.  Value is 
                                              SPICE_EK_VARSIZ for 
                                              variable-length strings. 
   
                                  size:       Column entry size; this is 
                                              the number of array 
                                              elements in a column 
                                              entry. The value is 
                                              SPICE_EK_VARSIZ for
                                              variable-size columns.  

                                  indexd:     Index flag; value is 
                                              SPICETRUE if the column is 
                                              indexed, SPICEFALSE 
                                              otherwise. 

                                  nullok:     Null flag; value is 
                                              SPICETRUE if the column 
                                              may contain null values, 
                                              SPICEFALSE otherwise. 



         SpiceEKSegSum         EK segment summary.  This structure 
                               contains user interface level descriptive
                               information.  The structure contains the 
                               following members:

                                  tabnam      The name of the table to 
                                              which the segment belongs. 

                                  nrows       The number of rows in the
                                              segment. 

                                  ncols       The number of columns in
                                              the segment.

                                  cnames      An array of names of 
                                              columns in the segment. 
                                              Column names may contain 
                                              as many as SPICE_EK_CNAMSZ 
                                              characters. The array 
                                              contains room for 
                                              SPICE_EK_MXCLSG column
                                              names.
   
                                  cdescrs     An array of column 
                                              attribute descriptors of
                                              type SpiceEKAttDsc.
                                              The array contains room 
                                              for SPICE_EK_MXCLSG 
                                              descriptors.  The Ith
                                              descriptor corresponds to
                                              the column whose name is
                                              the Ith element of the 
                                              array cnames.

-Literature_References

   None.

-Author_and_Institution

   N.J. Bachman       (JPL)
   
-Restrictions

   None.
      
-Version

   -CSPICE Version 2.0.0 27-JUL-2002 (NJB) 
   
      Defined SpiceEKDataType using SpiceDataType.  Removed declaration
      of enum _SpiceEKDataType.
   
   -CSPICE Version 1.0.0, 05-JUL-1999 (NJB)  

      Renamed _SpiceEKAttDsc member "class" to "cclass."  The
      former name is a reserved word in C++.


   -CSPICE Version 1.0.0, 24-FEB-1999 (NJB)  

*/

#ifndef HAVE_SPICE_EK_H

   #define HAVE_SPICE_EK_H
   
   
   
   /*
   Constants
   */

   /*
   Sizes of EK objects:
   */

   #define  SPICE_EK_CNAMSZ                 32
   #define  SPICE_EK_CSTRLN               ( SPICE_EK_CNAMSZ + 1 )
   #define  SPICE_EK_TNAMSZ                 64
   #define  SPICE_EK_TSTRLN               ( SPICE_EK_TNAMSZ + 1 )


   
   /*
   Maximum number of columns per segment:
   */
   
   #define  SPICE_EK_MXCLSG                 100


   /*
   Maximum length of string indicating data type:
   */
   
   #define  SPICE_EK_TYPLEN                 4
   
   
   /*
   Query-related limits (see header for details):
   */
   
   #define  SPICE_EK_MAXQRY                 2000
   #define  SPICE_EK_MAXQSEL                50
   #define  SPICE_EK_MAXQTAB                10
   #define  SPICE_EK_MAXQCON                1000
   #define  SPICE_EK_MAXQJOIN               10
   #define  SPICE_EK_MAXQJCON               100
   #define  SPICE_EK_MAXQORD                10
   #define  SPICE_EK_MAXQTOK                500
   #define  SPICE_EK_MAXQNUM                100
   #define  SPICE_EK_MAXQCLN                SPICE_EK_MAXQRY
   #define  SPICE_EK_MAXQSTR                1024
   
   
   
   /*
   Code indicating "variable size":
   */
   #define  SPICE_EK_VARSIZ               (-1)
   


   /*
   Data type codes:
   */
   typedef  SpiceDataType  SpiceEKDataType;
   
   
   
   /*
   SELECT clause expression type codes:
   */
   enum _SpiceEKExprClass{ SPICE_EK_EXP_COL  = 0, 
                           SPICE_EK_EXP_FUNC = 1, 
                           SPICE_EK_EXP_EXPR = 2 };

   typedef  enum _SpiceEKExprClass SpiceEKExprClass;



   /*
   EK column attribute descriptor:
   */
   
   struct _SpiceEKAttDsc 
   
      {  SpiceInt         cclass;
         SpiceEKDataType  dtype;
         SpiceInt         strlen;
         SpiceInt         size;
         SpiceBoolean     indexd;
         SpiceBoolean     nullok;  };
   
   typedef struct _SpiceEKAttDsc  SpiceEKAttDsc;
   
   
   
   /*
   EK segment summary:
   */
   
   struct _SpiceEKSegSum 
   
      { SpiceChar        tabnam [SPICE_EK_TSTRLN];
        SpiceInt         nrows;
        SpiceInt         ncols;
        SpiceChar        cnames [SPICE_EK_MXCLSG][SPICE_EK_CSTRLN];
        SpiceEKAttDsc    cdescrs[SPICE_EK_MXCLSG];                    };
          
   typedef struct _SpiceEKSegSum  SpiceEKSegSum;


#endif

