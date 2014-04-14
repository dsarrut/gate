/*----------------------
  GATE version name: gate_v6

  Copyright (C): OpenGATE Collaboration

  This software is distributed under the terms
  of the GNU Lesser General  Public Licence (LGPL)
  See GATE/LICENSE.txt for further details
  ----------------------*/

#include "GateConfiguration.h"
#include "GateEcatSystem.hh"
#include "GateClockDependentMessenger.hh"
#include "GateBox.hh"
#include "GateSystemComponent.hh"
#include "GateVolumePlacement.hh"
#include "GateCoincidenceSorter.hh"
#include "GateToSinogram.hh"
#include "GateSinoToEcat7.hh"
#include "GateDigitizer.hh"
#include "GateOutputMgr.hh"

#include "G4UnitsTable.hh"

// Constructor
GateEcatSystem::GateEcatSystem(const G4String& itsName)
  : GateVSystem( itsName , true ),
    m_gateToSinogram(0)
{
  // Set up a messenger
  m_messenger = new GateClockDependentMessenger(this);
  m_messenger->SetDirectoryGuidance(G4String("Controls the system '") + GetObjectName() + "'" );

  // Define the scanner components
  GateBoxComponent* blockComponent   = new GateBoxComponent("block",GetBaseComponent(),this);
  /*GateArrayComponent* arrayComponent =*/ new GateArrayComponent("crystal",blockComponent,this);

  // Integrate a coincidence sorter into the digitizer
  G4double coincidenceWindow = 10.* ns;
  GateDigitizer* digitizer = GateDigitizer::GetInstance();
  GateCoincidenceSorter* coincidenceSorter = new GateCoincidenceSorter(digitizer,"Coincidences",coincidenceWindow);
  digitizer->StoreNewCoincidenceSorter(coincidenceSorter);

  // Insert a sinogram maker and a ECAT7 writer into the output manager
  GateOutputMgr *outputMgr = GateOutputMgr::GetInstance();
  m_gateToSinogram = new GateToSinogram("sinogram", outputMgr,this,GateOutputMgr::GetDigiMode());
  outputMgr->AddOutputModule((GateVOutputModule*)m_gateToSinogram);
#ifdef GATE_USE_ECAT7
  m_gateSinoToEcat7 = new GateSinoToEcat7("ecat7", outputMgr,this,GateOutputMgr::GetDigiMode());
  outputMgr->AddOutputModule((GateVOutputModule*)m_gateSinoToEcat7);
#endif

  SetOutputIDName((char *)"gantryID",0);
  SetOutputIDName((char *)"blockID",1);
  SetOutputIDName((char *)"crystalID",2);

}



// Destructor
GateEcatSystem::~GateEcatSystem()
{
  delete m_messenger;
}



/*  Method overloading the base-class virtual method Describe().
    This methods prints-out a description of the system, which is
    optimised for creating ECAT header files

    indent: the print-out indentation (cosmetic parameter)
*/
void GateEcatSystem::Describe(size_t indent)
{
  GateVSystem::Describe(indent);
  PrintToStream(G4cout,true);
}



/* Method overloading the base-class virtual method Describe().
   This methods prints out description of the system to a stream.
   It is essentially to be used by the class GateToLMF, but it may also be used by Describe()

   aStream: the output stream
   doPrintNumbers: tells whether we print-out the volume numbers in addition to their dimensions
*/
void GateEcatSystem::PrintToStream(std::ostream& aStream,G4bool doPrintNumbers)
{
  aStream << " >> geometrical design type: " << 1 << G4endl;

  aStream << " >> ring diameter: " << G4BestUnit( 2*ComputeInternalRadius() ,"Length")   << G4endl;

  GateBoxComponent* blockComponent = FindBoxCreatorComponent("block");

  G4double blockAxialPitch = blockComponent->GetLinearRepeatVector().z();
  aStream << " >> block axial pitch: " << G4BestUnit( blockAxialPitch ,"Length")  	  << G4endl;

  G4double blockAzimuthalPitch = blockComponent->GetAngularRepeatPitch();
  aStream << " >> block azimuthal pitch: " << blockAzimuthalPitch/degree << " degree"  	 	  << G4endl;

  G4double blockTangentialSize = blockComponent->GetBoxLength(1) ;
  G4double blockAxialSize      = blockComponent->GetBoxLength(2) ;
  aStream << " >> block tangential size: " << G4BestUnit( blockTangentialSize ,"Length")  	  << G4endl;
  aStream << " >> block axial size: " << G4BestUnit( blockAxialSize ,"Length")  	      	  << G4endl;

  GateArrayComponent* crystalComponent = FindArrayComponent("crystal");

  G4double crystalTangentialSize = crystalComponent->GetBoxLength(1);
  G4double crystalAxialSize      = crystalComponent->GetBoxLength(2);
  aStream << " >> crystal axial size: " << G4BestUnit( crystalAxialSize ,"Length")  	      	  << G4endl;
  aStream << " >> crystal tangential size: " << G4BestUnit( crystalTangentialSize ,"Length")  	  << G4endl;

  G4ThreeVector crystalPitchVector = crystalComponent->GetRepeatVector();
  aStream << " >> crystal axial pitch: " << G4BestUnit( crystalPitchVector.z() ,"Length")    	  << G4endl;
  aStream << " >> crystal tangential pitch: " << G4BestUnit( crystalPitchVector.y() ,"Length")    	  << G4endl;


  if (doPrintNumbers) {
    aStream << " >> axial nb of blocks: " << blockComponent->GetLinearRepeatNumber()     	           << G4endl;
    aStream << " >> azimuthal nb of blocks: " << blockComponent->GetAngularRepeatNumber()   	   << G4endl;
    aStream << " >> axial nb of crystals: " << crystalComponent->GetRepeatNumber(2) 	      	   << G4endl;
    aStream << " >> tangential nb of crystals: " << crystalComponent->GetRepeatNumber(1) 	      	   << G4endl;
  }
}


// Compute the internal radius of the crystal ring.
G4double GateEcatSystem::ComputeInternalRadius()
{
  // Compute the radius to the center of the block
  GateBoxComponent *block = FindBoxCreatorComponent("block");
  GateVolumePlacement *blockMove = block->FindPlacementMove();
  G4double radius = blockMove ? blockMove->GetTranslation().x() : 0.;

  // Decrease by the block half-length to get the internal radius
  radius -=  .5 * block->GetBoxLength(0);

  // Add all the offsets between innermost edges
  radius += FindComponent("crystal")->ComputeOffset(0,GateSystemComponent::align_left,GateSystemComponent::align_left);


  return radius;
}
