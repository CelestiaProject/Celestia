/*

-Header_File SpiceCel.h ( CSPICE Cell definitions )

-Abstract

   Perform CSPICE definitions for the SpiceCell data type.
            
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

   CELLS
   
-Particulars

   This header defines structures, macros, and enumerated types that
   may be referenced in application code that calls CSPICE cell
   functions.
 
   CSPICE cells are data structures that implement functionality
   parallel to that of the cell abstract data type in SPICELIB.  In
   CSPICE, a cell is a C structure containing bookkeeping information,
   including a pointer to an associated data array.
 
   For numeric data types, the data array is simply a SPICELIB-style
   cell, including a valid control area.  For character cells, the data
   array has the same number of elements as the corresponding
   SPICELIB-style cell, but the contents of the control area are not
   maintained, and the data elements are null-terminated C-style
   strings.

   CSPICE cells should be declared using the declaration macros
   provided in this header file.  See the table of macros below.

  
      Structures
      ==========
   
         Name                  Description
         ----                  ----------
   
         SpiceCell             Structure containing CSPICE cell metadata.
         
                               The members are:

                                  dtype:     Data type of cell: character,
                                             integer, or double precision.

                                             dtype has type 
                                             SpiceCellDataType.

                                  length:    For character cells, the 
                                             declared length of the 
                                             cell's string array.
 
                                  size:      The maximum number of data
                                             items that can be stored in
                                             the cell's data array.

                                  card:      The cell's "cardinality": the
                                             number of data items currently
                                             present in the cell.

                                  isSet:     Boolean flag indicating whether
                                             the cell is a CSPICE set.  
                                             Sets have no duplicate data 
                                             items, and their data items are
                                             stored in increasing order.

                                  adjust:    Boolean flag indicating whether
                                             the cell's data area has 
                                             adjustable size.  Adjustable
                                             size cell data areas are not 
                                             currently implemented.

                                  init:      Boolean flag indicating whether
                                             the cell has been initialized.

                                  base:      is a void pointer to the 
                                             associated data array.  base
                                             points to the start of the
                                             control area of this array.

                                  data:      is a void pointer to the 
                                             first data slot in the 
                                             associated data array.  This
                                             slot is the element following
                                             the control area.

 
         ConstSpiceCell        A const SpiceCell.
         

  

      Declaration Macros
      ==================      

      Name                                            Description
      ----                                            ----------

      SPICECHAR_CELL ( name, size, length )           Declare a
                                                      character CSPICE
                                                      cell having cell
                                                      name name,
                                                      maximum cell
                                                      cardinality size,
                                                      and string length
                                                      length.  The
                                                      macro declares
                                                      both the cell and
                                                      the associated
                                                      data array. The
                                                      name of the data
                                                      array begins with
                                                      "SPICE_".
 
                                                         
      SPICEDOUBLE_CELL ( name, size )                 Like SPICECHAR_CELL,
                                                      but declares a 
                                                      double precision
                                                      cell.

  
      SPICEINT_CELL ( name, size )                    Like
                                                      SPICECHAR_CELL,
                                                      but declares an
                                                      integer cell.

      Assignment Macros
      =================      

      Name                                            Description
      ----                                            ----------
      SPICE_CELL_SET_C( item, i, cell )               Assign the ith
                                                      element of a
                                                      character cell.
                                                      Arguments cell
                                                      and item are
                                                      pointers.
 
      SPICE_CELL_SET_D( item, i, cell )               Assign the ith
                                                      element of a
                                                      double precision
                                                      cell. Argument
                                                      cell is a
                                                      pointer.
 
      SPICE_CELL_SET_I( item, i, cell )               Assign the ith
                                                      element of an
                                                      integer cell.
                                                      Argument cell is
                                                      a pointer.

 
      Fetch Macros
      ==============      

      Name                                            Description
      ----                                            ----------
      SPICE_CELL_GET_C( cell, i, lenout, item )       Fetch the ith
                                                      element from a
                                                      character cell.
                                                      Arguments cell
                                                      and item are
                                                      pointers.
                                                      Argument lenout
                                                      is the available
                                                      space in item.
 
      SPICE_CELL_GET_D( cell, i, item )               Fetch the ith
                                                      element from a
                                                      double precision
                                                      cell. Arguments
                                                      cell and item are
                                                      pointers.

      SPICE_CELL_GET_I( cell, i, item )               Fetch the ith
                                                      element from an
                                                      integer cell.
                                                      Arguments cell
                                                      and item are
                                                      pointers.
      Element Pointer Macros
      ======================      

      Name                                            Description
      ----                                            ----------
      SPICE_CELL_ELEM_C( cell, i )                    Macro evaluates
                                                      to a SpiceChar
                                                      pointer to the
                                                      ith data element
                                                      of a character
                                                      cell. Argument
                                                      cell is a
                                                      pointer.

      SPICE_CELL_ELEM_D( cell, i )                    Macro evaluates
                                                      to a SpiceDouble
                                                      pointer to the
                                                      ith data element
                                                      of a double
                                                      precision cell.
                                                      Argument cell is
                                                      a pointer.

      SPICE_CELL_ELEM_I( cell, i )                    Macro evaluates
                                                      to a SpiceInt
                                                      pointer to the
                                                      ith data element
                                                      of an integer
                                                      cell. Argument
                                                      cell is a
                                                      pointer.

-Literature_References

   None.

-Author_and_Institution

   N.J. Bachman       (JPL)
   
-Restrictions

   None.
      
-Version

   -CSPICE Version 1.0.0, 22-AUG-2002 (NJB)  

*/
#ifndef HAVE_SPICE_CELLS_H

   #define HAVE_SPICE_CELLS_H
      

   /*
   Data type codes:
   */
   typedef enum _SpiceDataType  SpiceCellDataType;


   /*
   Cell structure:
   */
   struct _SpiceCell
   
      {  SpiceCellDataType  dtype;
         SpiceInt           length;
         SpiceInt           size;
         SpiceInt           card;
         SpiceBoolean       isSet;
         SpiceBoolean       adjust;
         SpiceBoolean       init;
         void             * base;
         void             * data;  };     
        
   typedef struct _SpiceCell  SpiceCell;

   typedef const SpiceCell    ConstSpiceCell;


   /*
   SpiceCell control area size: 
   */
   #define SPICE_CELL_CTRLSZ         6


   /*
   Declaration macros: 
   */                   
                                           
   #define SPICECHAR_CELL( name, size, length )                             \
                                                                            \
      static SpiceChar SPICE_CELL_##name[SPICE_CELL_CTRLSZ + size][length]; \
                                                                            \
      static SpiceCell name =                                               \
                                                                            \
        { SPICE_CHR,                                                        \
          length,                                                           \
          size,                                                             \
          0,                                                                \
          SPICETRUE,                                                        \
          SPICEFALSE,                                                       \
          SPICEFALSE,                                                       \
          (void *) &(SPICE_CELL_##name),                                    \
          (void *) &(SPICE_CELL_##name[SPICE_CELL_CTRLSZ])  }
        

   #define SPICEDOUBLE_CELL( name, size )                                   \
                                                                            \
      static SpiceDouble SPICE_CELL_##name [SPICE_CELL_CTRLSZ + size];      \
                                                                            \
      static SpiceCell name =                                               \
                                                                            \
        { SPICE_DP,                                                         \
          0,                                                                \
          size,                                                             \
          0,                                                                \
          SPICETRUE,                                                        \
          SPICEFALSE,                                                       \
          SPICEFALSE,                                                       \
          (void *) &(SPICE_CELL_##name),                                    \
          (void *) &(SPICE_CELL_##name[SPICE_CELL_CTRLSZ])  }
       

   #define SPICEINT_CELL( name, size )                                      \
                                                                            \
      static SpiceInt SPICE_CELL_##name [SPICE_CELL_CTRLSZ + size];         \
                                                                            \
      static SpiceCell name =                                               \
                                                                            \
        { SPICE_INT,                                                        \
          0,                                                                \
          size,                                                             \
          0,                                                                \
          SPICETRUE,                                                        \
          SPICEFALSE,                                                       \
          SPICEFALSE,                                                       \
          (void *) &(SPICE_CELL_##name),                                    \
          (void *) &(SPICE_CELL_##name[SPICE_CELL_CTRLSZ])  }
           

   /*
   Access macros for individual elements: 
   */

   /*
   Data element pointer macros: 
   */

   #define SPICE_CELL_ELEM_C( cell, i )                                     \
                                                                            \
       (  ( (SpiceChar    *) (cell)->data ) + (i)*( (cell)->length )  )      


   #define SPICE_CELL_ELEM_D( cell, i )                                     \
                                                                            \
       (  ( (SpiceDouble  *) (cell)->data )[(i)]  )         


   #define SPICE_CELL_ELEM_I( cell, i )                                     \
                                                                            \
       (  ( (SpiceInt     *) (cell)->data )[(i)]  )         


   /*
   "Fetch" macros: 
   */

   #define SPICE_CELL_GET_C( cell, i, lenout, item )                        \
                                                                            \
       {                                                                    \
          SpiceInt    nBytes;                                               \
                                                                            \
          nBytes   =    brckti_c ( (cell)->length,  0, (lenout-1)  )        \
                     *  sizeof   ( SpiceChar );                             \
                                                                            \
          memmove ( (item),  SPICE_CELL_ELEM_C((cell), (i)),  nBytes );     \
                                                                            \
          item[nBytes] = NULLCHAR;                                          \
       }  


   #define SPICE_CELL_GET_D( cell, i, item )                                \
                                                                            \
       (  (*item) = ( (SpiceDouble *) (cell)->data)[i]  )    


   #define SPICE_CELL_GET_I( cell, i, item )                                \
                                                                            \
       (  (*item) = ( (SpiceInt    *) (cell)->data)[i]  )         


   /*
   Assignment macros: 
   */

   #define SPICE_CELL_SET_C( item, i, cell )                                \
                                                                            \
       {                                                                    \
          SpiceChar   * sPtr;                                               \
          SpiceInt      nBytes;                                             \
                                                                            \
          nBytes   =    brckti_c ( strlen(item), 0, (cell)->length - 1 )    \
                      * sizeof   ( SpiceChar );                             \
                                                                            \
          sPtr     =    SPICE_CELL_ELEM_C((cell), (i));                     \
                                                                            \
          memmove ( sPtr,  (item),  nBytes );                               \
                                                                            \
          sPtr[nBytes] = NULLCHAR;                                          \
       }  


   #define SPICE_CELL_SET_D( item, i, cell )                                \
                                                                            \
       (  ( (SpiceDouble *) (cell)->data)[i]  =  (item) )    


   #define SPICE_CELL_SET_I( item, i, cell )                                \
                                                                            \
       (  ( (SpiceInt    *) (cell)->data)[i]  =  (item) )         


   /*
   The enum SpiceTransDir is used to indicate language translation
   direction:  C to Fortran or vice versa. 
   */
   enum _SpiceTransDir { C2F = 0, F2C = 1 };
   
   typedef enum  _SpiceTransDir SpiceTransDir;


#endif

