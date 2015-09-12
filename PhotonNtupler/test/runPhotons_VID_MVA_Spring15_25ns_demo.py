import FWCore.ParameterSet.Config as cms

process = cms.Process("TestPhotons")

process.load("FWCore.MessageService.MessageLogger_cfi")

process.load("Configuration.StandardSequences.GeometryRecoDB_cff")

process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_cff")
# NOTE: the pick the right global tag!
#    for Spring15 scenario 50ns: global tag is MCRUN2_74_V9A
#    for Spring15 scenario 25ns: global tag is MCRUN2_74_V9
#  as a rule, find the global tag in the DAS under the Configs for given dataset
#  When lazy tools run (e.g. for photon ID), add ::All to the global tag.
process.GlobalTag.globaltag = 'MCRUN2_74_V9::All'

#
# Define input data to read
#
process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(10000) )

inputFilesAOD = cms.untracked.vstring(
    # AOD test files from 
    # /GJet_Pt-40toInf_DoubleEMEnriched_MGG-80toInf_TuneCUETP8M1_13TeV_Pythia8/RunIISpring15DR74-Asympt25ns_MCRUN2_74_V9-v1/AODSIM
       '/store/mc/RunIISpring15DR74/GJet_Pt-40toInf_DoubleEMEnriched_MGG-80toInf_TuneCUETP8M1_13TeV_Pythia8/AODSIM/Asympt25ns_MCRUN2_74_V9-v1/00000/00AF2C5E-C604-E511-8648-485B3989720C.root',
       '/store/mc/RunIISpring15DR74/GJet_Pt-40toInf_DoubleEMEnriched_MGG-80toInf_TuneCUETP8M1_13TeV_Pythia8/AODSIM/Asympt25ns_MCRUN2_74_V9-v1/00000/02020DE3-CE05-E511-BCF2-008CFA197B44.root',
       '/store/mc/RunIISpring15DR74/GJet_Pt-40toInf_DoubleEMEnriched_MGG-80toInf_TuneCUETP8M1_13TeV_Pythia8/AODSIM/Asympt25ns_MCRUN2_74_V9-v1/00000/02D0CA05-C204-E511-813C-7845C4FC3653.root',
    )    

inputFilesMiniAOD = cms.untracked.vstring(
    # MiniAOD test files from 
    # /GJet_Pt-40toInf_DoubleEMEnriched_MGG-80toInf_TuneCUETP8M1_13TeV_Pythia8/RunIISpring15DR74-Asympt25ns_MCRUN2_74_V9-v1/MINIAODSIM
       '/store/mc/RunIISpring15DR74/GJet_Pt-40toInf_DoubleEMEnriched_MGG-80toInf_TuneCUETP8M1_13TeV_Pythia8/MINIAODSIM/Asympt25ns_MCRUN2_74_V9-v1/00000/04904448-3D06-E511-AB50-00266CFCE03C.root',
       '/store/mc/RunIISpring15DR74/GJet_Pt-40toInf_DoubleEMEnriched_MGG-80toInf_TuneCUETP8M1_13TeV_Pythia8/MINIAODSIM/Asympt25ns_MCRUN2_74_V9-v1/00000/06A2ACA3-7005-E511-B165-008CFA0A58B4.root',
       '/store/mc/RunIISpring15DR74/GJet_Pt-40toInf_DoubleEMEnriched_MGG-80toInf_TuneCUETP8M1_13TeV_Pythia8/MINIAODSIM/Asympt25ns_MCRUN2_74_V9-v1/00000/06E82640-9605-E511-9165-001EC9ADCBEF.root',
    )

# Set up input/output depending on the format
# You can list here either AOD or miniAOD files, but not both types mixed
#
useAOD = False

if useAOD == True :
    inputFiles = inputFilesAOD
    outputFile = "photon_ntuple.root"
    print("AOD input files are used")
else :
    inputFiles = inputFilesMiniAOD
    outputFile = "photon_ntuple_mini.root"
    print("MiniAOD input files are used")
process.source = cms.Source ("PoolSource", fileNames = inputFiles )                             

#
# Set up photon ID (VID framework)
#

from PhysicsTools.SelectorUtils.tools.vid_id_tools import *
# turn on VID producer, indicate data format  to be
# DataFormat.AOD or DataFormat.MiniAOD, as appropriate 
if useAOD == True :
    dataFormat = DataFormat.AOD
else :
    dataFormat = DataFormat.MiniAOD

switchOnVIDPhotonIdProducer(process, dataFormat)

# define which IDs we want to produce
my_id_modules = ['RecoEgamma.PhotonIdentification.Identification.mvaPhotonID_Spring15_25ns_nonTrig_V2_cff']

#add them to the VID producer
for idmod in my_id_modules:
    setupAllVIDIdsInModule(process,idmod,setupVIDPhotonSelection)

#
# Configure an example module for user analysis with photons
#

process.ntupler = cms.EDAnalyzer(
    'PhotonNtuplerVIDwithMVADemo',
    # The module automatically detects AOD vs miniAOD, so we configure both
    #
    # Common to all formats objects
    #
    # ... none ...
    #
    # Objects specific to AOD format
    #
    photons = cms.InputTag("gedPhotons"),
    genParticles = cms.InputTag("genParticles"),
    #
    # Objects specific to MiniAOD format
    #
    photonsMiniAOD = cms.InputTag("slimmedPhotons"),
    genParticlesMiniAOD = cms.InputTag("prunedGenParticles"),
    #
    # ID decisions (common to all formats)
    #
    # (the names of the ValueMaps for just decision and full info are the same,
    # they are distinguished by the type of the info)
    phoMediumIdBoolMap = cms.InputTag("egmPhotonIDs:mvaPhoID-Spring15-25ns-nonTrig-V2-wp90"),
    phoMediumIdFullInfoMap = cms.InputTag("egmPhotonIDs:mvaPhoID-Spring15-25ns-nonTrig-V2-wp90"),
    # This is a fairly verbose mode if switched on, with full cut flow 
    # diagnostics for each candidate. Use it in a low event count test job.
    phoIdVerbose = cms.bool(False),
    #
    # ValueMaps with MVA results
    #
    mvaValuesMap     = cms.InputTag("photonMVAValueMapProducer:PhotonMVAEstimatorRun2Spring15NonTrig25nsV2Values"),
    mvaCategoriesMap = cms.InputTag("photonMVAValueMapProducer:PhotonMVAEstimatorRun2Spring15NonTrig25nsV2Categories")
    )

process.TFileService = cms.Service("TFileService",
                                   fileName = cms.string( outputFile )
                                   )

# Make sure to add the ID sequence upstream from the user analysis module
process.p = cms.Path(process.egmPhotonIDSequence * process.ntupler)

