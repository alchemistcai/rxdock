/***********************************************************************
 * The rDock program was developed from 1998 - 2006 by the software team
 * at RiboTargets (subsequently Vernalis (R&D) Ltd).
 * In 2006, the software was licensed to the University of York for
 * maintenance and distribution.
 * In 2012, Vernalis and the University of York agreed to release the
 * program as Open Source software.
 * This version is licensed under GNU-LGPL version 3.0 with support from
 * the University of Barcelona.
 * http://rdock.sourceforge.net/
 ***********************************************************************/

#include <sstream>

#include "rxdock/ElementFileSource.h"
#include "rxdock/FileError.h"

using namespace rxdock;

// Constructors
// ElementFileSource::ElementFileSource(const char* fileName) :
//  BaseFileSource(fileName)
//{
//  _RBTOBJECTCOUNTER_CONSTR_("ElementFileSource");
//}

ElementFileSource::ElementFileSource(const std::string &fileName)
    : BaseFileSource(fileName), m_dHBondRadiusIncr(0.0),
      m_dImplicitRadiusIncr(0.0) {
  _RBTOBJECTCOUNTER_CONSTR_("ElementFileSource");
}

// Destructor
ElementFileSource::~ElementFileSource() {
  ClearElementDataCache();
  _RBTOBJECTCOUNTER_DESTR_("ElementFileSource");
}

////////////////////////////////////////
// Public methods
std::string ElementFileSource::GetTitle() {
  Parse();
  return m_strTitle;
}

std::string ElementFileSource::GetVersion() {
  Parse();
  return m_strVersion;
}

unsigned int ElementFileSource::GetNumElements() {
  Parse();
  return m_elementNameMap.size();
}

// List of element names
std::vector<std::string> ElementFileSource::GetElementNameList() {
  Parse();
  std::vector<std::string> elementNameList;
  elementNameList.reserve(m_elementNameMap.size());
  for (StringElementDataMapConstIter iter = m_elementNameMap.begin();
       iter != m_elementNameMap.end(); iter++)
    elementNameList.push_back((*iter).first);
  return elementNameList;
}

// List of atomic numbers
std::vector<int> ElementFileSource::GetAtomicNumberList() {
  Parse();
  std::vector<int> atomicNumberList;
  atomicNumberList.reserve(m_atomicNumberMap.size());
  for (IntElementDataMapConstIter iter = m_atomicNumberMap.begin();
       iter != m_atomicNumberMap.end(); iter++)
    atomicNumberList.push_back((*iter).first);
  return atomicNumberList;
}

// Get element data for a given element name, throws error if not found
ElementData
ElementFileSource::GetElementData(const std::string &strElementName) {
  Parse();
  StringElementDataMapIter iter = m_elementNameMap.find(strElementName);
  if (iter != m_elementNameMap.end())
    return (*iter).second;
  else
    throw FileMissingParameter(_WHERE_, "Element " + strElementName +
                                            " not found in " + GetFileName());
}

// Get element data for a given atomic number, throws error if not found
ElementData ElementFileSource::GetElementData(int nAtomicNumber) {
  Parse();
  IntElementDataMapIter iter = m_atomicNumberMap.find(nAtomicNumber);
  if (iter != m_atomicNumberMap.end())
    return (*iter).second;
  else {
    std::ostringstream ostr;
    ostr << "Atomic number " << nAtomicNumber << " not found in "
         << GetFileName() << std::ends;
    std::string strError(ostr.str());
    // delete ostr.str();
    throw FileMissingParameter(_WHERE_, strError);
  }
}

// Check if given element name is present
bool ElementFileSource::isElementNamePresent(
    const std::string &strElementName) {
  Parse();
  StringElementDataMapIter iter = m_elementNameMap.find(strElementName);
  if (iter != m_elementNameMap.end())
    return true;
  else
    return false;
}

// Check if given atomic number is present
bool ElementFileSource::isAtomicNumberPresent(int nAtomicNumber) {
  Parse();
  IntElementDataMapIter iter = m_atomicNumberMap.find(nAtomicNumber);
  if (iter != m_atomicNumberMap.end())
    return true;
  else
    return false;
}

// Get vdw radius increment for hydrogen-bonding donors
double ElementFileSource::GetHBondRadiusIncr() {
  Parse();
  return m_dHBondRadiusIncr;
}

// Get vdw radius increment for atoms with implicit hydrogens
double ElementFileSource::GetImplicitRadiusIncr() {
  Parse();
  return m_dImplicitRadiusIncr;
}

// Private methods
// Pure virtual in BaseFileSource - needs to be defined here
void ElementFileSource::Parse() {
  const std::string strKey = "RBT_ELEMENT_FILE_V1.01";
  const std::string strTitleKey = "TITLE ";
  const std::string strVersionKey = "VERSION ";
  const std::string strElementKey = "ELEMENT";
  const std::string strHBondRadiusKey = "HBOND_RADIUS_INCREMENT";
  const std::string strImplicitRadiusKey = "IMPLICIT_RADIUS_INCREMENT";

  // Only parse if we haven't already done so
  if (!m_bParsedOK) {
    ClearElementDataCache(); // Clear current cache
    Read();                  // Read the file

    try {
      FileRecListIter fileIter = m_lineRecs.begin();
      FileRecListIter fileEnd = m_lineRecs.end();

      //////////////////////////////////////////////////////////
      // 1. Check for RBT string on line 1
      if ((*fileIter).find(strKey) != 0)
        throw FileParseError(_WHERE_, "Missing " + strKey + " string in " +
                                          GetFileName());

      //////////////////////////////////////////////////////////
      // 2. Parse the rest of file
      while (++fileIter != fileEnd) {
        // Ignore blank lines and comment lines
        if (((*fileIter).length() == 0) || ((*fileIter).at(0) == '#')) {
          continue;
        }
        // Check for Title record
        else if ((*fileIter).find(strTitleKey) == 0) {
          m_strTitle = *fileIter;
          m_strTitle.erase(0, strTitleKey.length());
        }
        // Check for Version record
        else if ((*fileIter).find(strVersionKey) == 0) {
          m_strVersion = *fileIter;
          m_strVersion.erase(0, strVersionKey.length());
        }
        // Check for HBond radius increment
        else if ((*fileIter).find(strHBondRadiusKey) == 0) {
          std::string strDummy;
          std::istringstream istr((*fileIter).c_str());
          istr >> strDummy >> m_dHBondRadiusIncr;
        }
        // Check for Implicit radius increment
        else if ((*fileIter).find(strImplicitRadiusKey) == 0) {
          std::string strDummy;
          std::istringstream istr((*fileIter).c_str());
          istr >> strDummy >> m_dImplicitRadiusIncr;
        }
        // Check for element record
        else if ((*fileIter).find(strElementKey) == 0) {
          ElementData elemData;
          std::string strDummy;
          std::istringstream istr((*fileIter).c_str());
          istr >> strDummy >> elemData.atomicNo >> elemData.element >>
              elemData.minVal >> elemData.maxVal >> elemData.commonVal >>
              elemData.mass >> elemData.vdwRadius;
          // Store the data in two maps, one indexed by element name, the other
          // by atomic number
          m_elementNameMap[elemData.element] = elemData;
          m_atomicNumberMap[elemData.atomicNo] = elemData;
        }
      }
      //////////////////////////////////////////////////////////
      // If we get this far everything is OK
      m_bParsedOK = true;
    }

    catch (Error &error) {
      ClearElementDataCache();
      throw; // Rethrow the Error
    }
  }
}

void ElementFileSource::ClearElementDataCache() {
  m_elementNameMap.clear();
  m_atomicNumberMap.clear();
  m_strTitle = "";
  m_strVersion = "";
  m_dHBondRadiusIncr = 0.0;
  m_dImplicitRadiusIncr = 0.0;
}
