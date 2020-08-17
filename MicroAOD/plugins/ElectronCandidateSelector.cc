#include "FWCore/Framework/interface/MakerMacros.h"
#include "CommonTools/UtilAlgos/interface/SingleObjectSelector.h"
#include "CommonTools/UtilAlgos/interface/StringCutObjectSelector.h"
#include "flashgg/DataFormats/interface/Electron.h"

#include "CommonTools/UtilAlgos/interface/ObjectSelectorStream.h"
#include "CommonTools/UtilAlgos/interface/SingleElementCollectionSelectorPlusEvent.h"

#include "CommonTools/UtilAlgos/interface/SortCollectionSelector.h"

typedef SingleObjectSelector <
edm::View<flashgg::Electron>,
    StringCutObjectSelector<flashgg::Electron, true>,
    std::vector<flashgg::Electron>
    > ElectronSelector;


#include "flashgg/MicroAOD/interface/CutBasedElectronObjectSelector.h"
typedef ObjectSelectorStream <
SingleElementCollectionSelectorPlusEvent <
edm::View<flashgg::Electron>,
    flashgg::CutBasedElectronObjectSelector,
    std::vector<flashgg::Electron>
    >,
    std::vector<flashgg::Electron> > GenericElectronSelector;

DEFINE_FWK_MODULE( ElectronSelector );
DEFINE_FWK_MODULE( GenericElectronSelector );


// Local Variables:
// mode:c++
// indent-tabs-mode:nil
// tab-width:4
// c-basic-offset:4
// End:
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4

