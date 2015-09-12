#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "TString.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TCanvas.h"
#include "TCut.h"

#include <vector>
#include <cmath>

const int nWP = 5;
enum WpType {
  WP_VETO = 0,
  WP_LOOSE,
  WP_MEDIUM, 
  WP_TIGHT,
  WP_HEEP};
const TString wpName[nWP] = 
  {"Veto", "Loose", "Medium", "Tight", "HEEP"};

const TString treename = "ntupler/ElectronTree";
const TString fname = "../ElectronNtupler/test/electron_ntuple_mini.root";

bool verbose = false;
bool smallEventCount = false;
bool useWeights = true; // Unused variable???

const float ptmin = 20;
const float ptmax = 200;

void computeElectronIDEfficiency(WpType wp, bool useDzAndConv = true)
{

  // This statement below should not be needed, but in one particular node I had to
  // add it, somehow the vector header was not loaded automatically there.
  gROOT->ProcessLine("#include <vector>"); 

  if( (int)wp<0 || (int)wp>nWP) assert(0);
  printf("Compute the efficiencies for the working point %s\n", wpName[wp].Data());

  //
  // Find the tree
  //
  TFile *file1 = new TFile(fname);
  if( !file1 )
    assert(0);
  TTree *tree = (TTree*)file1->Get(treename);
  if( !tree )
    assert(0);

  // Event-level variables:
  int nEle;
  // Per-electron variables
  // Kinematics
  std::vector <float> *pt = 0;     
  std::vector <float> *eta = 0;    
  std::vector <float> *dz = 0;    
  std::vector <int> *passConversionVeto = 0;    
  std::vector <int> *passChosenId = 0;    
  std::vector <int> *isTrue = 0;    

  // Declare branches
  TBranch *b_nEle = 0;
  TBranch *b_pt = 0;
  TBranch *b_eta = 0;
  TBranch *b_dz = 0;
  TBranch *b_passConversionVeto = 0;
  TBranch *b_passChosenId = 0;
  TBranch *b_isTrue = 0;

  // Connect variables and branches to the tree with the data
  tree->SetBranchAddress("nEle", &nEle, &b_nEle);
  tree->SetBranchAddress("pt", &pt, &b_pt);
  tree->SetBranchAddress("eta", &eta, &b_eta);

  if( useDzAndConv ) {
    tree->SetBranchAddress("dz", &dz, &b_dz);
    tree->SetBranchAddress("passConversionVeto", &passConversionVeto, &b_passConversionVeto);
  }

  if( wp == WP_VETO )
    tree->SetBranchAddress("passVetoId", &passChosenId, &b_passChosenId);
  else if( wp == WP_LOOSE )
    tree->SetBranchAddress("passLooseId", &passChosenId, &b_passChosenId);
  else if( wp == WP_MEDIUM)
    tree->SetBranchAddress("passMediumId", &passChosenId, &b_passChosenId);
  else if( wp == WP_TIGHT)
    tree->SetBranchAddress("passTightId", &passChosenId, &b_passChosenId);
  else if( wp == WP_HEEP)
    tree->SetBranchAddress("passHEEPId", &passChosenId, &b_passChosenId);
  else
    assert(0);
  tree->SetBranchAddress("isTrue", &isTrue, &b_isTrue);

  // Declare event counters for efficiency calculations
  double sumSignalDenomEB = 0;
  double sumSignalNumEB   = 0;
  double sumSignalDenomEE = 0;
  double sumSignalNumEE   = 0;
  double sumBackDenomEB = 0;
  double sumBackNumEB   = 0;
  double sumBackDenomEE = 0;
  double sumBackNumEE   = 0;

  double sumSignalDenomEBErr2 = 0;
  double sumSignalNumEBErr2   = 0;
  double sumSignalDenomEEErr2 = 0;
  double sumSignalNumEEErr2   = 0;
  double sumBackDenomEBErr2 = 0;
  double sumBackNumEBErr2   = 0;
  double sumBackDenomEEErr2 = 0;
  double sumBackNumEEErr2   = 0;

  // 
  // Loop over events
  //
  UInt_t maxEvents = tree->GetEntries();
  if( smallEventCount )
    maxEvents = std::min((float)10000, (float)maxEvents);
  if(verbose)
    printf("Start loop over events, total events = %lld\n", 
           tree->GetEntries() );
  for(UInt_t ievent = 0; ievent < maxEvents; ievent++){

    if( ievent%100000 == 0){
      printf("."); fflush(stdout);
    }
    Long64_t tentry = tree->LoadTree(ievent);

    // Load the value of the number of the electrons in the event    
    b_nEle->GetEntry(tentry);
    if(verbose)
      printf("Event %d, number of electrons %u\n", ievent, nEle);

    // Get data for all electrons in this event, only vars of interest
    b_pt->GetEntry(tentry);
    b_eta->GetEntry(tentry);
    if( useDzAndConv) {
      b_dz->GetEntry(tentry);
      b_passConversionVeto->GetEntry(tentry);
    }
    b_passChosenId->GetEntry(tentry);
    b_isTrue->GetEntry(tentry);

    // Loop over electrons
    for(int iele = 0; iele < nEle; iele++){
      
      // Preselection
      if( !(pt->at(iele) > ptmin && pt->at(iele) < ptmax ) ) continue;
      if( fabs(eta->at(iele))>1.4442 && fabs(eta->at(iele))<1.566) continue;
      if( fabs(eta->at(iele))>2.5 ) continue;
      // 
      if( useDzAndConv ) {
        if( ! (fabs(dz->at(iele))<1) ) continue;
        if( ! (passConversionVeto->at(iele) == 1) ) continue;
      }

      bool isBarrel = (fabs(eta->at(iele)) < 1.479);
      
      if(verbose)
	printf(" next electron pt=%f id decision is %d\n",
	       pt->at(iele),
	       passChosenId->at(iele));

      double weight = 1.0;

      int isMatched = isTrue->at(iele);

      // Fill signal counters
      if( isMatched == 1 ) {

	if( isBarrel ) {
	  sumSignalDenomEB += weight;
	  sumSignalDenomEBErr2 += weight*weight;
	  if( passChosenId->at(iele) ) {
	    sumSignalNumEB += weight;
	    sumSignalNumEBErr2 += weight*weight;
	  }
	} else {
	  sumSignalDenomEE += weight;
	  sumSignalDenomEEErr2 += weight*weight;          
	  if( passChosenId->at(iele) ) {
	    sumSignalNumEE += weight;
	    sumSignalNumEEErr2 += weight*weight;
	  }
	  
	}// end barrel / endcap
      } // end if signal
      
      // Fill background counters
      // (use unmatched (0) or matched to non-prompt non-tau electrons (3) )
      if( isMatched==0 || isMatched==3 ) {
	
        if( isBarrel ) {
	  
          sumBackDenomEB += weight;
          sumBackDenomEBErr2 += weight*weight;
          if( passChosenId->at(iele) ) {
            sumBackNumEB += weight;
            sumBackNumEBErr2 += weight*weight;
          }
        } else {
          sumBackDenomEE += weight;
          sumBackDenomEEErr2 += weight*weight;    
          if( passChosenId->at(iele) ) {
            sumBackNumEE += weight;
            sumBackNumEEErr2 += weight*weight;
          }
	  
        }// end barrel / endcap
      } // end if background
      
    } // end loop over photons
    
    
  }// end loop over events  

  // DEBUG
  //if(verbose){
    printf("barrel signal pass=%.1f   total=%.1f\n", sumSignalNumEB, sumSignalDenomEB);
    printf("endcap signal pass=%.1f   total=%.1f\n", sumSignalNumEE, sumSignalDenomEE);
    //}

  printf("\nEfficiencies for the working point %s\n", wpName[wp].Data());

  // Compute signal efficiencies
  double effSignalEB = sumSignalNumEB / sumSignalDenomEB;
  double effSignalEBErr = sqrt( sumSignalDenomEBErr2 
                                * effSignalEB*(1-effSignalEB)
                                /(sumSignalDenomEB*sumSignalDenomEB) );
  printf("Signal barrel efficiency: %5.1f +- %5.1f %%\n", 
         effSignalEB*100, effSignalEBErr*100 );

  double effSignalEE = sumSignalNumEE / sumSignalDenomEE;
  double effSignalEEErr = sqrt( sumSignalDenomEEErr2 
                                * effSignalEE*(1-effSignalEE)
                                /(sumSignalDenomEE*sumSignalDenomEE) );
  printf("Signal endcap efficiency: %5.1f +- %5.1f %%\n", 
         effSignalEE*100, effSignalEEErr*100 );

  // Compute background efficiencies
  double effBackEB = sumBackNumEB / sumBackDenomEB;
  double effBackEBErr = sqrt( sumBackDenomEBErr2 
                                * effBackEB*(1-effBackEB)
                                /(sumBackDenomEB*sumBackDenomEB) );
  printf("Background barrel efficiency: %5.1f +- %5.1f %%\n", 
         effBackEB*100, effBackEBErr*100 );

  double effBackEE = sumBackNumEE / sumBackDenomEE;
  double effBackEEErr = sqrt( sumBackDenomEEErr2 
                                * effBackEE*(1-effBackEE)
                                /(sumBackDenomEE*sumBackDenomEE) );
  printf("Background endcap efficiency: %5.1f +- %5.1f %%\n", 
         effBackEE*100, effBackEEErr*100 );

}
  
