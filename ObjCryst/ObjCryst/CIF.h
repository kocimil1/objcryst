#ifndef _CIF_H
#define _CIF_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <list>
#include <map>
#include <set>

#include "Quirks/ci_string.h"

namespace ObjCryst
{
/// Convert one CIF value to a floating-point value
/// Return 0 if no value can be converted (e.g. if '.' or '?' is encountered)
float CIFNumeric2Float(const std::string &s);
/// Convert one CIF value to a floating-point value
/// Return 0 if no value can be converted (e.g. if '.' or '?' is encountered)
int CIFNumeric2Int(const std::string &s);

/** The CIFData class holds all the information from a \e single data_ block from a cif file.
* 
* It is a placeholder for all comments, item and loop data, as raw string copied from
* a cif file.
*
* It is also used to interpret this data to extract parts of the cif data, i.e.
* only part of the core cif dictionnary are recognized. CIF tags currently recognized
* include ("tag1 > tag2" means tag1 is preferred to tag2 when extracting the info, only one is reported):
*  - crystal name: _chemical_name_systematic > _chemical_name_mineral > _chemical_name_structure_type > _chemical_name_common
*  - crystal formula: _chemical_formula_analytical > _chemical_formula_structural > _chemical_formula_iupac > _chemical_formula_moiety
*  - unit cell:  _cell_length_{a,b,c} ; _cell_angle_{alpha,beta,gamma}
*  - spacegroup number: _space_group_IT_number > _symmetry_Int_Tables_number
*  - spacegroup Hall symbol: _space_group_name_Hall > _symmetry_space_group_name_Hall
*  - spacegroup Hermann-Mauguin symbol:_space_group_name_H-M_alt > _symmetry_space_group_name_H-M
*  - atom coordinates: _atom_site_fract_{x} ; _atom_site_Cartn_{x,y,z}
*  - atom occupancy: _atom_site_occupancy
*  - atom label & symbol: _atom_site_type_symbol ; _atom_site_label
*
* If another data field is needed, it is possible to directly access the string data 
* (CIFData::mvComment , CIFData::mvItem and CIFData::mvLoop) to search for the correct tags.
*
* Cartesian coordinates are stored in Angstroems, angles in radians.
*/
class CIFData
{
   public:
      CIFData();
      
      /// Extract lattice parameters, spacegroup (symbol or number), atomic positions,
      /// chemical name and formula if available.
      /// All other data is ignored
      void ExtractAll(const bool verbose=false);
      /// Extract name & formula for the crystal
      void ExtractName(const bool verbose=false);
      /// Extract unit cell
      void ExtractUnitCell(const bool verbose=false);
      /// Extract spacegroup number or symbol
      void ExtractSpacegroup(const bool verbose=false);
      /// Extract all atomic positions. Will generate cartesian from fractional
      /// coordinates or vice-versa if only cartesian coordinates are available.
      void ExtractAtomicPositions(const bool verbose=false);
      /// Generate fractional coordinates from cartesian ones for all atoms
      /// CIFData::CalcMatrices() must be called first
      void Cartesian2FractionalCoord();
      /// Generate cartesian coordinates from fractional ones for all atoms
      /// CIFData::CalcMatrices() must be called first
      void Fractional2CartesianCoord();
      /// Convert from fractional to cartesian coordinates
      /// CIFData::CalcMatrices() must be called first
      void f2c(float &x,float &y, float &z);
      /// Convert from cartesia to fractional coordinates
      /// CIFData::CalcMatrices() must be called first
      void c2f(float &x,float &y, float &z);
      /// Calculate real space transformation matrices
      /// requires unit cell parameters
      void CalcMatrices(const bool verbose=false);
      /// Comments from CIF file, in the order they were read
      std::list<std::string> mvComment;
      /// Individual CIF items
      std::map<ci_string,std::string> mvItem;
      /// CIF Loop data
      std::map<std::set<ci_string>,std::map<ci_string,std::vector<std::string> > > mvLoop;
      /// Lattice parameters, in ansgtroem and degrees - vector size is 0 if no
      /// parameters have been obtained yet.
      std::vector<float> mvLatticePar;
      /// Spacegroup number from International Tables (_space_group_IT_number), or -1.
      unsigned int mSpacegroupNumberIT;
      /// Spacegroup Hall symbol (or empty string) (_space_group_name_Hall)
      std::string mSpacegroupSymbolHall;
      /// Spacegroup Hermann-Mauguin symbol (or empty string) (_space_group_name_H-M_alt)
      std::string mSpacegroupHermannMauguin;
      /// Crystal name. Or empty string if none is available.
      std::string mName;
      /// Formula. Or empty string if none is available.
      std::string mFormula;
      /// Atom record 
      struct CIFAtom
      {
         CIFAtom();
         /// Label of the atom, or empty string (_atom_site_label).
         std::string mLabel;
         /// Symbol of the atom, or empty string (_atom_type_symbol or _atom_site_type_symbol).
         std::string mSymbol;
         /// Fractionnal coordinates (_atom_site_fract_{x,y,z}) or empty vector.
         std::vector<float> mCoordFrac;
         /// Cartesian coordinates in Angstroem (_atom_site_Cartn_{x,y,z}) or empty vector.
         /// Transformation to fractionnal coordinates currently assumes 
         /// "a parallel to x; b in the plane of y and z" (see _atom_sites_Cartn_transform_axes)
         std::vector<float> mCoordCart;
         /// Site occupancy, or -1
         float mOccupancy;
      };
      /// Atoms, if any are found
      std::vector<CIFAtom> mvAtom;
      /// Fractionnal2Cartesian matrix
      float mOrthMatrix[3][3];
      /// Cartesian2Fractionnal matrix
      float mOrthMatrixInvert[3][3];
};

/** Main CIF class - parses the stream and separates data blocks, comments, items, loops.
* All values are stored as string, and Each CIF block is stored in a separate CIFData object.
* No interpretaion is made here - this must be done from all CIFData objects.
*/
class CIF
{
   public:
      /// Creates the CIF object from a stream
      ///
      /// \param interpret: if true, interpret all data blocks. See CIFData::ExtractAll()
      CIF(std::istream &in, const bool interpret=true,const bool verbose=false);
   //private:
      /// Separate the file in data blocks and parse them to sort tags, loops and comments.
      /// All is stored in the original strings.
      void Parse();
      /// The input stream
      std::istream *mStream;
      /// The full cif file, line by line
      std::list<std::string> mvLine;
      /// The data blocks, after parsing. The key is the name of the data block
      std::map<std::string,CIFData> mvData;
      /// Global comments, outside and data block
      std::list<std::string> mvComment;
};

/// Extract one Crystal object from a CIF.
/// Returns a null pointer if no crystal structure could be extracted
/// (the minimum data is the unit cell parameters).
Crystal* CreateCrystalFromCIF(std::istream &in);

}

#endif
