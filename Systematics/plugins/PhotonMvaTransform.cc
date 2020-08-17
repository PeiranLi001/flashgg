#include "flashgg/Systematics/interface/ObjectSystMethodBinnedByFunctor.h"
#include "DataFormats/Common/interface/Handle.h"
#include "DataFormats/Common/interface/PtrVector.h"
#include "flashgg/DataFormats/interface/Photon.h"
#include "CommonTools/Utils/interface/StringCutObjectSelector.h"

#include "TGraph.h"
#include "TFile.h"

namespace flashgg {

    class PhotonMvaTransform: public ObjectSystMethodBinnedByFunctor<flashgg::Photon, int>
    {

    public:
        typedef StringCutObjectSelector<Photon, true> selector_type;

        PhotonMvaTransform( const edm::ParameterSet &conf, edm::ConsumesCollector && iC, const GlobalVariablesComputer *gv );
        void applyCorrection( flashgg::Photon &y, int syst_shift ) override;
        std::string shiftLabel( int ) const override;

    private:
        selector_type overall_range_;
        edm::FileInPath correctionFile_;
        std::vector<std::unique_ptr<TGraph> > corrections_;
    };

    PhotonMvaTransform::PhotonMvaTransform( const edm::ParameterSet &conf, edm::ConsumesCollector && iC, const GlobalVariablesComputer *gv ) :
        ObjectSystMethodBinnedByFunctor( conf, std::forward<edm::ConsumesCollector>(iC), gv ),
        overall_range_( conf.getParameter<std::string>( "OverallRange" ) )
    {
        correctionFile_ = conf.getParameter<edm::FileInPath>("CorrectionFile");
        // std::cout<<correctionFile_.fullPath().c_str()<<std::endl;
        TFile* f = TFile::Open(correctionFile_.fullPath().c_str());
        //        f->Print();
        corrections_.emplace_back( (TGraph*)((TGraph*) f->Get("trasfhebup"))->Clone());
        corrections_.emplace_back((TGraph*)((TGraph*) f->Get("trasfhebdown"))->Clone() );
        corrections_.emplace_back((TGraph*)((TGraph*) f->Get("trasfheeup"))->Clone() );
        corrections_.emplace_back((TGraph*)((TGraph*) f->Get("trasfheedown"))->Clone() );
        f->Close();
    }

    std::string PhotonMvaTransform::shiftLabel( int syst_value ) const
    {
        std::string result;
        if( syst_value == 0 ) {
            result = Form( "%sCentral", label().c_str() );
        } else if( syst_value > 0 ) {
            result = Form( "%sUp%.2dsigma", label().c_str(), syst_value );
        } else {
            result = Form( "%sDown%.2dsigma", label().c_str(), -1 * syst_value );
        }
        return result;
    }

    void PhotonMvaTransform::applyCorrection( flashgg::Photon &y, int syst_shift )
    {
        //        if(syst_shift==0) return;
        // cout << "syst_shift: " << syst_shift << endl; 
        // debug_ = 1;
        if( overall_range_( y ) ) { 
            auto val_err = binContents( y );
            // cout << "val_err: " << val_err << endl;
            float shift_val = val_err.first[0]; 
            if (!applyCentralValue()) shift_val = 0.;
            //            std::cout<<"syst shift is "<<syst_shift<<std::endl;
            int correctionIndex = -1;
            if (y.isEB()) {
                if (syst_shift > 0)
                    correctionIndex = 0;
                else 
                    correctionIndex = 1;

            } else {
                if (syst_shift > 0)
                    correctionIndex = 2;
                else
                    correctionIndex = 3;
            }
            if (correctionIndex == -1) std::cerr << "ERROR: not enough corrections defined for IDMVA" << std::endl;            
         
            auto beforeMap = y.phoIdMvaD();
            std::vector<edm::Ptr<reco::Vertex> > keys;
            if( debug_ ) {
                std::cout << " PhoID MVA Syst shift from transformation : Photon has pt= " << y.pt() << " eta=" << y.eta();
                std::cout << "     MVA VALUES BEFORE: ";
                for(auto keyval = beforeMap.begin(); keyval != beforeMap.end(); ++keyval) {
                    keys.push_back( keyval->first );
                    std::cout << keyval->second << " ";
                }
                std::cout << std::endl;
            }
            edm::Ptr<reco::Vertex> vtx;
            float shift =0., mvaVal=0.;
            for(auto keyval = beforeMap.begin(); keyval != beforeMap.end(); ++keyval) {
                vtx = keyval->first;
                mvaVal = keyval->second;
                shift = shift_val + (mvaVal - corrections_[correctionIndex]->Eval(mvaVal))*abs(syst_shift);
                // cout < "mvaVal: " << mvaVal << endl;
                // # diphoton vertex x = -0.0245905
                // # diphoton vertex y = 0.0705721
                // # diphoton vertex z = -0.969548
                //if(abs(shift)>0){
                //    std::cout << "shift: " << shift << endl;
                //    std::cout << " PhoID MVA Syst shift from transformation : Photon has pt= " << y.pt() << " eta=" << y.eta();
                //    std::cout << "     MVA VALUES BEFORE: ";
                //    for(auto keyval = beforeMap.begin(); keyval != beforeMap.end(); ++keyval) {
                //        keys.push_back( keyval->first );
                //        std::cout << keyval->second << " ";
                //    }
                //    std::cout << std::endl;                    
                //}
                //     cout << "---------------------------------" << endl;
                //     // cout << "shift: " << shift << endl;
                //     std::map<edm::Ptr<reco::Vertex>, float> test = y.phoIdMvaD();
                //     cout << "mva val = " << test[vtx] << endl; 
                //     cout << "being shifted by : " << endl;
                //     cout << shift << endl;
                //     cout << "" << endl;
                //     cout << "Vertex x: " << vtx->x() << endl;
                //     cout << "Vertex y: " << vtx->y() << endl;
                //     cout << "Vertex z: " << vtx->z() << endl;
                // //     // cout << "y.phoIdMvaD()[vtx]: " << y.phoIdMvaD()[vtx] << endl;
                //     cout << "---------------------------------" << endl;
                // //     cout << "mvaVal: " << mvaVal << endl;
                // //     cout << "corrections_[correctionIndex]->Eval(mvaVal): " << corrections_[correctionIndex]->Eval(mvaVal) << endl;
                // //     cout << "abs(syst_shift): " << abs(syst_shift) << endl;
                // //     cout << "shift_val: " << shift_val << endl;
                // //     cout << "(mvaVal - corrections_[correctionIndex]->Eval(mvaVal))*abs(syst_shift) = " << (mvaVal - corrections_[correctionIndex]->Eval(mvaVal))*abs(syst_shift) << endl;
                // }
                
                y.shiftMvaValueBy(shift, vtx);
            }
/*
            if(abs(shift)>0){
                std::cout << "shift: " << shift << endl;
                auto afterMap = y.phoIdMvaD();
                std::cout << "     MVA VALUES AFTER: ";
                for(auto key = keys.begin() ; key != keys.end() ; ++key ) {
                    std::cout << afterMap.at(*key) << " ";
                }
                std::cout << std::endl;                
            }*/

            if ( debug_) {
                auto afterMap = y.phoIdMvaD();
                std::cout << "     MVA VALUES AFTER: ";
                for(auto key = keys.begin() ; key != keys.end() ; ++key ) {
                    std::cout << afterMap.at(*key) << " ";
                }
                std::cout << std::endl;
            }
        }
    }
}

DEFINE_EDM_PLUGIN( FlashggSystematicPhotonMethodsFactory,
                   flashgg::PhotonMvaTransform,
                   "FlashggPhotonMvaTransform" );
// Local Variables:
// mode:c++
// indent-tabs-mode:nil
// tab-width:4
// c-basic-offset:4
// End:
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4


// Begin processing the 43rd record. Run 1, Event 1544, LumiSection 16 on stream 0 at 17-Mar-2020 10:47:21.847 CET
// ---------------------------------
// mva val = 0.952591
// being shifted by : 
// 0.00398087

// Vertex x: -0.0245905
// Vertex y: 0.0705721
// Vertex z: -0.969548
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0257838
// Vertex y: 0.068611
// Vertex z: 2.66782
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0257798
// Vertex y: 0.0703014
// Vertex z: 0.867136
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0256227
// Vertex y: 0.0681952
// Vertex z: -5.82667
// ---------------------------------
// ---------------------------------
// mva val = 0.956082
// being shifted by : 
// 0.00382864

// Vertex x: -0.0261578
// Vertex y: 0.0726024
// Vertex z: 3.14332
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0264752
// Vertex y: 0.0694427
// Vertex z: -2.8736
// ---------------------------------
// ---------------------------------
// mva val = 0.886621
// being shifted by : 
// 0.00875094

// Vertex x: -0.023153
// Vertex y: 0.0696622
// Vertex z: -0.691108
// ---------------------------------
// ---------------------------------
// mva val = 0.95022
// being shifted by : 
// 0.00412429

// Vertex x: -0.0240079
// Vertex y: 0.0708847
// Vertex z: -3.76259
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0241331
// Vertex y: 0.068394
// Vertex z: 6.7068
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0278316
// Vertex y: 0.0708665
// Vertex z: 4.66578
// ---------------------------------
// ---------------------------------
// mva val = 0.85489
// being shifted by : 
// 0.0117297

// Vertex x: -0.0247293
// Vertex y: 0.0681341
// Vertex z: 0.0395433
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0266014
// Vertex y: 0.0709273
// Vertex z: 4.58677
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0256405
// Vertex y: 0.0690473
// Vertex z: 4.19585
// ---------------------------------
// ---------------------------------
// mva val = 0.874301
// being shifted by : 
// 0.00987817

// Vertex x: -0.0313374
// Vertex y: 0.0680999
// Vertex z: -0.038734
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.02695
// Vertex y: 0.0670679
// Vertex z: 4.235
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0263528
// Vertex y: 0.063713
// Vertex z: 6.66545
// ---------------------------------
// ---------------------------------
// mva val = 0.956082
// being shifted by : 
// 0.00382864

// Vertex x: -0.029124
// Vertex y: 0.067214
// Vertex z: 3.32855
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0286521
// Vertex y: 0.0666014
// Vertex z: 9.67302
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.031186
// Vertex y: 0.0847825
// Vertex z: 2.54682
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.017899
// Vertex y: 0.0635979
// Vertex z: 3.89679
// ---------------------------------
// ---------------------------------
// mva val = 0.874301
// being shifted by : 
// 0.00987817

// Vertex x: -0.0241867
// Vertex y: 0.0679888
// Vertex z: 0.00367349
// ---------------------------------
// ---------------------------------
// mva val = 0.95022
// being shifted by : 
// 0.00412429

// Vertex x: -0.0238319
// Vertex y: 0.0774567
// Vertex z: -3.89929
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0222069
// Vertex y: 0.0725338
// Vertex z: -2.3325
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.02301
// Vertex y: 0.0710796
// Vertex z: 2.4042
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0351056
// Vertex y: 0.0578184
// Vertex z: -2.44508
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0248584
// Vertex y: 0.0751172
// Vertex z: 1.45528
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0304414
// Vertex y: 0.0691536
// Vertex z: -1.62724
// ---------------------------------
// ---------------------------------
// mva val = 0.95022
// being shifted by : 
// 0.00412429

// Vertex x: -0.0335603
// Vertex y: 0.0702266
// Vertex z: -3.86238
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0131435
// Vertex y: 0.0691479
// Vertex z: -4.97994
// ---------------------------------
// ---------------------------------
// mva val = 0.874301
// being shifted by : 
// 0.00987817

// Vertex x: -0.0349087
// Vertex y: 0.0755583
// Vertex z: -0.741475
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0466758
// Vertex y: 0.0696975
// Vertex z: 0.725605
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0312982
// Vertex y: 0.0684877
// Vertex z: 8.82161
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.025784
// Vertex y: 0.0715802
// Vertex z: 0.93881
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0330463
// Vertex y: 0.0546973
// Vertex z: 0.470302
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0250638
// Vertex y: 0.0719174
// Vertex z: -1.41643
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.028317
// Vertex y: 0.0661768
// Vertex z: 4.36176
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0175241
// Vertex y: 0.129554
// Vertex z: 3.60592
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.011143
// Vertex y: 0.0510278
// Vertex z: 5.95073
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0344007
// Vertex y: 0.0713908
// Vertex z: 7.17705
// ---------------------------------
// ---------------------------------
// mva val = 0.938357
// being shifted by : 
// 0.00476994

// Vertex x: -0.0245905
// Vertex y: 0.0705721
// Vertex z: -0.969548
// ---------------------------------
// ---------------------------------
// mva val = 0.951237
// being shifted by : 
// 0.00405803

// Vertex x: -0.0257838
// Vertex y: 0.068611
// Vertex z: 2.66782
// ---------------------------------
// ---------------------------------
// mva val = 0.853989
// being shifted by : 
// 0.011815

// Vertex x: -0.0257798
// Vertex y: 0.0703014
// Vertex z: 0.867136
// ---------------------------------
// ---------------------------------
// mva val = 0.926885
// being shifted by : 
// 0.00550291

// Vertex x: -0.0256227
// Vertex y: 0.0681952
// Vertex z: -5.82667
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0261578
// Vertex y: 0.0726024
// Vertex z: 3.14332
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0264752
// Vertex y: 0.0694427
// Vertex z: -2.8736
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.023153
// Vertex y: 0.0696622
// Vertex z: -0.691108
// ---------------------------------
// ---------------------------------
// mva val = 0.753042
// being shifted by : 
// 0.0224124

// Vertex x: -0.0240079
// Vertex y: 0.0708847
// Vertex z: -3.76259
// ---------------------------------
// ---------------------------------
// mva val = 0.951639
// being shifted by : 
// 0.00402784

// Vertex x: -0.0241331
// Vertex y: 0.068394
// Vertex z: 6.7068
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0278316
// Vertex y: 0.0708665
// Vertex z: 4.66578
// ---------------------------------
// ---------------------------------
// mva val = 0.941991
// being shifted by : 
// 0.00456135

// Vertex x: -0.0247293
// Vertex y: 0.0681341
// Vertex z: 0.0395433
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0266014
// Vertex y: 0.0709273
// Vertex z: 4.58677
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0256405
// Vertex y: 0.0690473
// Vertex z: 4.19585
// ---------------------------------
// ---------------------------------
// mva val = 0.941991
// being shifted by : 
// 0.00456135

// Vertex x: -0.0313374
// Vertex y: 0.0680999
// Vertex z: -0.038734
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.02695
// Vertex y: 0.0670679
// Vertex z: 4.235
// ---------------------------------
// ---------------------------------
// mva val = 0.951639
// being shifted by : 
// 0.00402784

// Vertex x: -0.0263528
// Vertex y: 0.063713
// Vertex z: 6.66545
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.029124
// Vertex y: 0.067214
// Vertex z: 3.32855
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0286521
// Vertex y: 0.0666014
// Vertex z: 9.67302
// ---------------------------------
// ---------------------------------
// mva val = 0.951237
// being shifted by : 
// 0.00405803

// Vertex x: -0.031186
// Vertex y: 0.0847825
// Vertex z: 2.54682
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.017899
// Vertex y: 0.0635979
// Vertex z: 3.89679
// ---------------------------------
// ---------------------------------
// mva val = 0.941991
// being shifted by : 
// 0.00456135

// Vertex x: -0.0241867
// Vertex y: 0.0679888
// Vertex z: 0.00367349
// ---------------------------------
// ---------------------------------
// mva val = 0.753042
// being shifted by : 
// 0.0224124

// Vertex x: -0.0238319
// Vertex y: 0.0774567
// Vertex z: -3.89929
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0222069
// Vertex y: 0.0725338
// Vertex z: -2.3325
// ---------------------------------
// ---------------------------------
// mva val = 0.951237
// being shifted by : 
// 0.00405803

// Vertex x: -0.02301
// Vertex y: 0.0710796
// Vertex z: 2.4042
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0351056
// Vertex y: 0.0578184
// Vertex z: -2.44508
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0248584
// Vertex y: 0.0751172
// Vertex z: 1.45528
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0304414
// Vertex y: 0.0691536
// Vertex z: -1.62724
// ---------------------------------
// ---------------------------------
// mva val = 0.753042
// being shifted by : 
// 0.0224124

// Vertex x: -0.0335603
// Vertex y: 0.0702266
// Vertex z: -3.86238
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0131435
// Vertex y: 0.0691479
// Vertex z: -4.97994
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0349087
// Vertex y: 0.0755583
// Vertex z: -0.741475
// ---------------------------------
// ---------------------------------
// mva val = 0.853989
// being shifted by : 
// 0.011815

// Vertex x: -0.0466758
// Vertex y: 0.0696975
// Vertex z: 0.725605
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0312982
// Vertex y: 0.0684877
// Vertex z: 8.82161
// ---------------------------------
// ---------------------------------
// mva val = 0.853989
// being shifted by : 
// 0.011815

// Vertex x: -0.025784
// Vertex y: 0.0715802
// Vertex z: 0.93881
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0330463
// Vertex y: 0.0546973
// Vertex z: 0.470302
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0250638
// Vertex y: 0.0719174
// Vertex z: -1.41643
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.028317
// Vertex y: 0.0661768
// Vertex z: 4.36176
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0175241
// Vertex y: 0.129554
// Vertex z: 3.60592
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.011143
// Vertex y: 0.0510278
// Vertex z: 5.95073
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0344007
// Vertex y: 0.0713908
// Vertex z: 7.17705
// ---------------------------------
// ---------------------------------
// mva val = 0.952591
// being shifted by : 
// -0.00397913

// Vertex x: -0.0245905
// Vertex y: 0.0705721
// Vertex z: -0.969548
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0257838
// Vertex y: 0.068611
// Vertex z: 2.66782
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0257798
// Vertex y: 0.0703014
// Vertex z: 0.867136
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0256227
// Vertex y: 0.0681952
// Vertex z: -5.82667
// ---------------------------------
// ---------------------------------
// mva val = 0.956082
// being shifted by : 
// -0.00384554

// Vertex x: -0.0261578
// Vertex y: 0.0726024
// Vertex z: 3.14332
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0264752
// Vertex y: 0.0694427
// Vertex z: -2.8736
// ---------------------------------
// ---------------------------------
// mva val = 0.886621
// being shifted by : 
// -0.00814958

// Vertex x: -0.023153
// Vertex y: 0.0696622
// Vertex z: -0.691108
// ---------------------------------
// ---------------------------------
// mva val = 0.95022
// being shifted by : 
// -0.00407496

// Vertex x: -0.0240079
// Vertex y: 0.0708847
// Vertex z: -3.76259
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0241331
// Vertex y: 0.068394
// Vertex z: 6.7068
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0278316
// Vertex y: 0.0708665
// Vertex z: 4.66578
// ---------------------------------
// ---------------------------------
// mva val = 0.85489
// being shifted by : 
// -0.0107569

// Vertex x: -0.0247293
// Vertex y: 0.0681341
// Vertex z: 0.0395433
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0266014
// Vertex y: 0.0709273
// Vertex z: 4.58677
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0256405
// Vertex y: 0.0690473
// Vertex z: 4.19585
// ---------------------------------
// ---------------------------------
// mva val = 0.874301
// being shifted by : 
// -0.00914983

// Vertex x: -0.0313374
// Vertex y: 0.0680999
// Vertex z: -0.038734
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.02695
// Vertex y: 0.0670679
// Vertex z: 4.235
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0263528
// Vertex y: 0.063713
// Vertex z: 6.66545
// ---------------------------------
// ---------------------------------
// mva val = 0.956082
// being shifted by : 
// -0.00384554

// Vertex x: -0.029124
// Vertex y: 0.067214
// Vertex z: 3.32855
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0286521
// Vertex y: 0.0666014
// Vertex z: 9.67302
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.031186
// Vertex y: 0.0847825
// Vertex z: 2.54682
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.017899
// Vertex y: 0.0635979
// Vertex z: 3.89679
// ---------------------------------
// ---------------------------------
// mva val = 0.874301
// being shifted by : 
// -0.00914983

// Vertex x: -0.0241867
// Vertex y: 0.0679888
// Vertex z: 0.00367349
// ---------------------------------
// ---------------------------------
// mva val = 0.95022
// being shifted by : 
// -0.00407496

// Vertex x: -0.0238319
// Vertex y: 0.0774567
// Vertex z: -3.89929
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0222069
// Vertex y: 0.0725338
// Vertex z: -2.3325
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.02301
// Vertex y: 0.0710796
// Vertex z: 2.4042
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0351056
// Vertex y: 0.0578184
// Vertex z: -2.44508
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0248584
// Vertex y: 0.0751172
// Vertex z: 1.45528
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0304414
// Vertex y: 0.0691536
// Vertex z: -1.62724
// ---------------------------------
// ---------------------------------
// mva val = 0.95022
// being shifted by : 
// -0.00407496

// Vertex x: -0.0335603
// Vertex y: 0.0702266
// Vertex z: -3.86238
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0131435
// Vertex y: 0.0691479
// Vertex z: -4.97994
// ---------------------------------
// ---------------------------------
// mva val = 0.874301
// being shifted by : 
// -0.00914983

// Vertex x: -0.0349087
// Vertex y: 0.0755583
// Vertex z: -0.741475
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0466758
// Vertex y: 0.0696975
// Vertex z: 0.725605
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0312982
// Vertex y: 0.0684877
// Vertex z: 8.82161
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.025784
// Vertex y: 0.0715802
// Vertex z: 0.93881
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0330463
// Vertex y: 0.0546973
// Vertex z: 0.470302
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0250638
// Vertex y: 0.0719174
// Vertex z: -1.41643
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.028317
// Vertex y: 0.0661768
// Vertex z: 4.36176
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0175241
// Vertex y: 0.129554
// Vertex z: 3.60592
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.011143
// Vertex y: 0.0510278
// Vertex z: 5.95073
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0344007
// Vertex y: 0.0713908
// Vertex z: 7.17705
// ---------------------------------
// ---------------------------------
// mva val = 0.938357
// being shifted by : 
// -0.00467031

// Vertex x: -0.0245905
// Vertex y: 0.0705721
// Vertex z: -0.969548
// ---------------------------------
// ---------------------------------
// mva val = 0.951237
// being shifted by : 
// -0.00403761

// Vertex x: -0.0257838
// Vertex y: 0.068611
// Vertex z: 2.66782
// ---------------------------------
// ---------------------------------
// mva val = 0.853989
// being shifted by : 
// -0.0108398

// Vertex x: -0.0257798
// Vertex y: 0.0703014
// Vertex z: 0.867136
// ---------------------------------
// ---------------------------------
// mva val = 0.926885
// being shifted by : 
// -0.00530599

// Vertex x: -0.0256227
// Vertex y: 0.0681952
// Vertex z: -5.82667
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0261578
// Vertex y: 0.0726024
// Vertex z: 3.14332
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0264752
// Vertex y: 0.0694427
// Vertex z: -2.8736
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.023153
// Vertex y: 0.0696622
// Vertex z: -0.691108
// ---------------------------------
// ---------------------------------
// mva val = 0.753042
// being shifted by : 
// -0.0200264

// Vertex x: -0.0240079
// Vertex y: 0.0708847
// Vertex z: -3.76259
// ---------------------------------
// ---------------------------------
// mva val = 0.951639
// being shifted by : 
// -0.00402308

// Vertex x: -0.0241331
// Vertex y: 0.068394
// Vertex z: 6.7068
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0278316
// Vertex y: 0.0708665
// Vertex z: 4.66578
// ---------------------------------
// ---------------------------------
// mva val = 0.941991
// being shifted by : 
// -0.00448821

// Vertex x: -0.0247293
// Vertex y: 0.0681341
// Vertex z: 0.0395433
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0266014
// Vertex y: 0.0709273
// Vertex z: 4.58677
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0256405
// Vertex y: 0.0690473
// Vertex z: 4.19585
// ---------------------------------
// ---------------------------------
// mva val = 0.941991
// being shifted by : 
// -0.00448821

// Vertex x: -0.0313374
// Vertex y: 0.0680999
// Vertex z: -0.038734
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.02695
// Vertex y: 0.0670679
// Vertex z: 4.235
// ---------------------------------
// ---------------------------------
// mva val = 0.951639
// being shifted by : 
// -0.00402308

// Vertex x: -0.0263528
// Vertex y: 0.063713
// Vertex z: 6.66545
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.029124
// Vertex y: 0.067214
// Vertex z: 3.32855
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0286521
// Vertex y: 0.0666014
// Vertex z: 9.67302
// ---------------------------------
// ---------------------------------
// mva val = 0.951237
// being shifted by : 
// -0.00403761

// Vertex x: -0.031186
// Vertex y: 0.0847825
// Vertex z: 2.54682
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.017899
// Vertex y: 0.0635979
// Vertex z: 3.89679
// ---------------------------------
// ---------------------------------
// mva val = 0.941991
// being shifted by : 
// -0.00448821

// Vertex x: -0.0241867
// Vertex y: 0.0679888
// Vertex z: 0.00367349
// ---------------------------------
// ---------------------------------
// mva val = 0.753042
// being shifted by : 
// -0.0200264

// Vertex x: -0.0238319
// Vertex y: 0.0774567
// Vertex z: -3.89929
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0222069
// Vertex y: 0.0725338
// Vertex z: -2.3325
// ---------------------------------
// ---------------------------------
// mva val = 0.951237
// being shifted by : 
// -0.00403761

// Vertex x: -0.02301
// Vertex y: 0.0710796
// Vertex z: 2.4042
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0351056
// Vertex y: 0.0578184
// Vertex z: -2.44508
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0248584
// Vertex y: 0.0751172
// Vertex z: 1.45528
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0304414
// Vertex y: 0.0691536
// Vertex z: -1.62724
// ---------------------------------
// ---------------------------------
// mva val = 0.753042
// being shifted by : 
// -0.0200264

// Vertex x: -0.0335603
// Vertex y: 0.0702266
// Vertex z: -3.86238
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0131435
// Vertex y: 0.0691479
// Vertex z: -4.97994
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0349087
// Vertex y: 0.0755583
// Vertex z: -0.741475
// ---------------------------------
// ---------------------------------
// mva val = 0.853989
// being shifted by : 
// -0.0108398

// Vertex x: -0.0466758
// Vertex y: 0.0696975
// Vertex z: 0.725605
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0312982
// Vertex y: 0.0684877
// Vertex z: 8.82161
// ---------------------------------
// ---------------------------------
// mva val = 0.853989
// being shifted by : 
// -0.0108398

// Vertex x: -0.025784
// Vertex y: 0.0715802
// Vertex z: 0.93881
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0330463
// Vertex y: 0.0546973
// Vertex z: 0.470302
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0250638
// Vertex y: 0.0719174
// Vertex z: -1.41643
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.028317
// Vertex y: 0.0661768
// Vertex z: 4.36176
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0175241
// Vertex y: 0.129554
// Vertex z: 3.60592
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.011143
// Vertex y: 0.0510278
// Vertex z: 5.95073
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0344007
// Vertex y: 0.0713908
// Vertex z: 7.17705
// ---------------------------------
// ---------------------------------
// mva val = 0.952591
// being shifted by : 
// 0.00398087

// Vertex x: -0.0245905
// Vertex y: 0.0705721
// Vertex z: -0.969548
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0257838
// Vertex y: 0.068611
// Vertex z: 2.66782
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0257798
// Vertex y: 0.0703014
// Vertex z: 0.867136
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0256227
// Vertex y: 0.0681952
// Vertex z: -5.82667
// ---------------------------------
// ---------------------------------
// mva val = 0.956082
// being shifted by : 
// 0.00382864

// Vertex x: -0.0261578
// Vertex y: 0.0726024
// Vertex z: 3.14332
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0264752
// Vertex y: 0.0694427
// Vertex z: -2.8736
// ---------------------------------
// ---------------------------------
// mva val = 0.886621
// being shifted by : 
// 0.00875094

// Vertex x: -0.023153
// Vertex y: 0.0696622
// Vertex z: -0.691108
// ---------------------------------
// ---------------------------------
// mva val = 0.95022
// being shifted by : 
// 0.00412429

// Vertex x: -0.0240079
// Vertex y: 0.0708847
// Vertex z: -3.76259
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0241331
// Vertex y: 0.068394
// Vertex z: 6.7068
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0278316
// Vertex y: 0.0708665
// Vertex z: 4.66578
// ---------------------------------
// ---------------------------------
// mva val = 0.85489
// being shifted by : 
// 0.0117297

// Vertex x: -0.0247293
// Vertex y: 0.0681341
// Vertex z: 0.0395433
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0266014
// Vertex y: 0.0709273
// Vertex z: 4.58677
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0256405
// Vertex y: 0.0690473
// Vertex z: 4.19585
// ---------------------------------
// ---------------------------------
// mva val = 0.874301
// being shifted by : 
// 0.00987817

// Vertex x: -0.0313374
// Vertex y: 0.0680999
// Vertex z: -0.038734
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.02695
// Vertex y: 0.0670679
// Vertex z: 4.235
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0263528
// Vertex y: 0.063713
// Vertex z: 6.66545
// ---------------------------------
// ---------------------------------
// mva val = 0.956082
// being shifted by : 
// 0.00382864

// Vertex x: -0.029124
// Vertex y: 0.067214
// Vertex z: 3.32855
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0286521
// Vertex y: 0.0666014
// Vertex z: 9.67302
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.031186
// Vertex y: 0.0847825
// Vertex z: 2.54682
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.017899
// Vertex y: 0.0635979
// Vertex z: 3.89679
// ---------------------------------
// ---------------------------------
// mva val = 0.874301
// being shifted by : 
// 0.00987817

// Vertex x: -0.0241867
// Vertex y: 0.0679888
// Vertex z: 0.00367349
// ---------------------------------
// ---------------------------------
// mva val = 0.95022
// being shifted by : 
// 0.00412429

// Vertex x: -0.0238319
// Vertex y: 0.0774567
// Vertex z: -3.89929
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0222069
// Vertex y: 0.0725338
// Vertex z: -2.3325
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.02301
// Vertex y: 0.0710796
// Vertex z: 2.4042
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0351056
// Vertex y: 0.0578184
// Vertex z: -2.44508
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0248584
// Vertex y: 0.0751172
// Vertex z: 1.45528
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0304414
// Vertex y: 0.0691536
// Vertex z: -1.62724
// ---------------------------------
// ---------------------------------
// mva val = 0.95022
// being shifted by : 
// 0.00412429

// Vertex x: -0.0335603
// Vertex y: 0.0702266
// Vertex z: -3.86238
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0131435
// Vertex y: 0.0691479
// Vertex z: -4.97994
// ---------------------------------
// ---------------------------------
// mva val = 0.874301
// being shifted by : 
// 0.00987817

// Vertex x: -0.0349087
// Vertex y: 0.0755583
// Vertex z: -0.741475
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0466758
// Vertex y: 0.0696975
// Vertex z: 0.725605
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0312982
// Vertex y: 0.0684877
// Vertex z: 8.82161
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.025784
// Vertex y: 0.0715802
// Vertex z: 0.93881
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0330463
// Vertex y: 0.0546973
// Vertex z: 0.470302
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0250638
// Vertex y: 0.0719174
// Vertex z: -1.41643
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.028317
// Vertex y: 0.0661768
// Vertex z: 4.36176
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0175241
// Vertex y: 0.129554
// Vertex z: 3.60592
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.011143
// Vertex y: 0.0510278
// Vertex z: 5.95073
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// 0.00348505

// Vertex x: -0.0344007
// Vertex y: 0.0713908
// Vertex z: 7.17705
// ---------------------------------
// ---------------------------------
// mva val = -0.997835
// being shifted by : 
// 0.00208949

// Vertex x: -0.0245905
// Vertex y: 0.0705721
// Vertex z: -0.969548
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0257838
// Vertex y: 0.068611
// Vertex z: 2.66782
// ---------------------------------
// ---------------------------------
// mva val = -0.997729
// being shifted by : 
// 0.002191

// Vertex x: -0.0257798
// Vertex y: 0.0703014
// Vertex z: 0.867136
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0256227
// Vertex y: 0.0681952
// Vertex z: -5.82667
// ---------------------------------
// ---------------------------------
// mva val = -0.997778
// being shifted by : 
// 0.00214447

// Vertex x: -0.0261578
// Vertex y: 0.0726024
// Vertex z: 3.14332
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0264752
// Vertex y: 0.0694427
// Vertex z: -2.8736
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.023153
// Vertex y: 0.0696622
// Vertex z: -0.691108
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0240079
// Vertex y: 0.0708847
// Vertex z: -3.76259
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0241331
// Vertex y: 0.068394
// Vertex z: 6.7068
// ---------------------------------
// ---------------------------------
// mva val = -0.996986
// being shifted by : 
// 0.00290809

// Vertex x: -0.0278316
// Vertex y: 0.0708665
// Vertex z: 4.66578
// ---------------------------------
// ---------------------------------
// mva val = -0.99668
// being shifted by : 
// 0.00320394

// Vertex x: -0.0247293
// Vertex y: 0.0681341
// Vertex z: 0.0395433
// ---------------------------------
// ---------------------------------
// mva val = -0.996986
// being shifted by : 
// 0.00290809

// Vertex x: -0.0266014
// Vertex y: 0.0709273
// Vertex z: 4.58677
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0256405
// Vertex y: 0.0690473
// Vertex z: 4.19585
// ---------------------------------
// ---------------------------------
// mva val = -0.99668
// being shifted by : 
// 0.00320394

// Vertex x: -0.0313374
// Vertex y: 0.0680999
// Vertex z: -0.038734
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.02695
// Vertex y: 0.0670679
// Vertex z: 4.235
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0263528
// Vertex y: 0.063713
// Vertex z: 6.66545
// ---------------------------------
// ---------------------------------
// mva val = -0.99712
// being shifted by : 
// 0.0027792

// Vertex x: -0.029124
// Vertex y: 0.067214
// Vertex z: 3.32855
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0286521
// Vertex y: 0.0666014
// Vertex z: 9.67302
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.031186
// Vertex y: 0.0847825
// Vertex z: 2.54682
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.017899
// Vertex y: 0.0635979
// Vertex z: 3.89679
// ---------------------------------
// ---------------------------------
// mva val = -0.99668
// being shifted by : 
// 0.00320394

// Vertex x: -0.0241867
// Vertex y: 0.0679888
// Vertex z: 0.00367349
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0238319
// Vertex y: 0.0774567
// Vertex z: -3.89929
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0222069
// Vertex y: 0.0725338
// Vertex z: -2.3325
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.02301
// Vertex y: 0.0710796
// Vertex z: 2.4042
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0351056
// Vertex y: 0.0578184
// Vertex z: -2.44508
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0248584
// Vertex y: 0.0751172
// Vertex z: 1.45528
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0304414
// Vertex y: 0.0691536
// Vertex z: -1.62724
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0335603
// Vertex y: 0.0702266
// Vertex z: -3.86238
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0131435
// Vertex y: 0.0691479
// Vertex z: -4.97994
// ---------------------------------
// ---------------------------------
// mva val = -0.99777
// being shifted by : 
// 0.00215132

// Vertex x: -0.0349087
// Vertex y: 0.0755583
// Vertex z: -0.741475
// ---------------------------------
// ---------------------------------
// mva val = -0.997729
// being shifted by : 
// 0.002191

// Vertex x: -0.0466758
// Vertex y: 0.0696975
// Vertex z: 0.725605
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0312982
// Vertex y: 0.0684877
// Vertex z: 8.82161
// ---------------------------------
// ---------------------------------
// mva val = -0.997729
// being shifted by : 
// 0.002191

// Vertex x: -0.025784
// Vertex y: 0.0715802
// Vertex z: 0.93881
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0330463
// Vertex y: 0.0546973
// Vertex z: 0.470302
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0250638
// Vertex y: 0.0719174
// Vertex z: -1.41643
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.028317
// Vertex y: 0.0661768
// Vertex z: 4.36176
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0175241
// Vertex y: 0.129554
// Vertex z: 3.60592
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.011143
// Vertex y: 0.0510278
// Vertex z: 5.95073
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0344007
// Vertex y: 0.0713908
// Vertex z: 7.17705
// ---------------------------------
// ---------------------------------
// mva val = 0.952591
// being shifted by : 
// -0.00397913

// Vertex x: -0.0245905
// Vertex y: 0.0705721
// Vertex z: -0.969548
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0257838
// Vertex y: 0.068611
// Vertex z: 2.66782
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0257798
// Vertex y: 0.0703014
// Vertex z: 0.867136
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0256227
// Vertex y: 0.0681952
// Vertex z: -5.82667
// ---------------------------------
// ---------------------------------
// mva val = 0.956082
// being shifted by : 
// -0.00384554

// Vertex x: -0.0261578
// Vertex y: 0.0726024
// Vertex z: 3.14332
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0264752
// Vertex y: 0.0694427
// Vertex z: -2.8736
// ---------------------------------
// ---------------------------------
// mva val = 0.886621
// being shifted by : 
// -0.00814958

// Vertex x: -0.023153
// Vertex y: 0.0696622
// Vertex z: -0.691108
// ---------------------------------
// ---------------------------------
// mva val = 0.95022
// being shifted by : 
// -0.00407496

// Vertex x: -0.0240079
// Vertex y: 0.0708847
// Vertex z: -3.76259
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0241331
// Vertex y: 0.068394
// Vertex z: 6.7068
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0278316
// Vertex y: 0.0708665
// Vertex z: 4.66578
// ---------------------------------
// ---------------------------------
// mva val = 0.85489
// being shifted by : 
// -0.0107569

// Vertex x: -0.0247293
// Vertex y: 0.0681341
// Vertex z: 0.0395433
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0266014
// Vertex y: 0.0709273
// Vertex z: 4.58677
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0256405
// Vertex y: 0.0690473
// Vertex z: 4.19585
// ---------------------------------
// ---------------------------------
// mva val = 0.874301
// being shifted by : 
// -0.00914983

// Vertex x: -0.0313374
// Vertex y: 0.0680999
// Vertex z: -0.038734
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.02695
// Vertex y: 0.0670679
// Vertex z: 4.235
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0263528
// Vertex y: 0.063713
// Vertex z: 6.66545
// ---------------------------------
// ---------------------------------
// mva val = 0.956082
// being shifted by : 
// -0.00384554

// Vertex x: -0.029124
// Vertex y: 0.067214
// Vertex z: 3.32855
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0286521
// Vertex y: 0.0666014
// Vertex z: 9.67302
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.031186
// Vertex y: 0.0847825
// Vertex z: 2.54682
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.017899
// Vertex y: 0.0635979
// Vertex z: 3.89679
// ---------------------------------
// ---------------------------------
// mva val = 0.874301
// being shifted by : 
// -0.00914983

// Vertex x: -0.0241867
// Vertex y: 0.0679888
// Vertex z: 0.00367349
// ---------------------------------
// ---------------------------------
// mva val = 0.95022
// being shifted by : 
// -0.00407496

// Vertex x: -0.0238319
// Vertex y: 0.0774567
// Vertex z: -3.89929
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0222069
// Vertex y: 0.0725338
// Vertex z: -2.3325
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.02301
// Vertex y: 0.0710796
// Vertex z: 2.4042
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0351056
// Vertex y: 0.0578184
// Vertex z: -2.44508
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0248584
// Vertex y: 0.0751172
// Vertex z: 1.45528
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0304414
// Vertex y: 0.0691536
// Vertex z: -1.62724
// ---------------------------------
// ---------------------------------
// mva val = 0.95022
// being shifted by : 
// -0.00407496

// Vertex x: -0.0335603
// Vertex y: 0.0702266
// Vertex z: -3.86238
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0131435
// Vertex y: 0.0691479
// Vertex z: -4.97994
// ---------------------------------
// ---------------------------------
// mva val = 0.874301
// being shifted by : 
// -0.00914983

// Vertex x: -0.0349087
// Vertex y: 0.0755583
// Vertex z: -0.741475
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0466758
// Vertex y: 0.0696975
// Vertex z: 0.725605
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0312982
// Vertex y: 0.0684877
// Vertex z: 8.82161
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.025784
// Vertex y: 0.0715802
// Vertex z: 0.93881
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0330463
// Vertex y: 0.0546973
// Vertex z: 0.470302
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0250638
// Vertex y: 0.0719174
// Vertex z: -1.41643
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.028317
// Vertex y: 0.0661768
// Vertex z: 4.36176
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0175241
// Vertex y: 0.129554
// Vertex z: 3.60592
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.011143
// Vertex y: 0.0510278
// Vertex z: 5.95073
// ---------------------------------
// ---------------------------------
// mva val = 0.966105
// being shifted by : 
// -0.00361323

// Vertex x: -0.0344007
// Vertex y: 0.0713908
// Vertex z: 7.17705
// ---------------------------------
// ---------------------------------
// mva val = -0.997835
// being shifted by : 
// -0.0256967

// Vertex x: -0.0245905
// Vertex y: 0.0705721
// Vertex z: -0.969548
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0257838
// Vertex y: 0.068611
// Vertex z: 2.66782
// ---------------------------------
// ---------------------------------
// mva val = -0.997729
// being shifted by : 
// -0.0269451

// Vertex x: -0.0257798
// Vertex y: 0.0703014
// Vertex z: 0.867136
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0256227
// Vertex y: 0.0681952
// Vertex z: -5.82667
// ---------------------------------
// ---------------------------------
// mva val = -0.997778
// being shifted by : 
// -0.0263729

// Vertex x: -0.0261578
// Vertex y: 0.0726024
// Vertex z: 3.14332
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0264752
// Vertex y: 0.0694427
// Vertex z: -2.8736
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.023153
// Vertex y: 0.0696622
// Vertex z: -0.691108
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0240079
// Vertex y: 0.0708847
// Vertex z: -3.76259
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0241331
// Vertex y: 0.068394
// Vertex z: 6.7068
// ---------------------------------
// ---------------------------------
// mva val = -0.996986
// being shifted by : 
// -0.0357639

// Vertex x: -0.0278316
// Vertex y: 0.0708665
// Vertex z: 4.66578
// ---------------------------------
// ---------------------------------
// mva val = -0.99668
// being shifted by : 
// -0.0394023

// Vertex x: -0.0247293
// Vertex y: 0.0681341
// Vertex z: 0.0395433
// ---------------------------------
// ---------------------------------
// mva val = -0.996986
// being shifted by : 
// -0.0357639

// Vertex x: -0.0266014
// Vertex y: 0.0709273
// Vertex z: 4.58677
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0256405
// Vertex y: 0.0690473
// Vertex z: 4.19585
// ---------------------------------
// ---------------------------------
// mva val = -0.99668
// being shifted by : 
// -0.0394023

// Vertex x: -0.0313374
// Vertex y: 0.0680999
// Vertex z: -0.038734
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.02695
// Vertex y: 0.0670679
// Vertex z: 4.235
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0263528
// Vertex y: 0.063713
// Vertex z: 6.66545
// ---------------------------------
// ---------------------------------
// mva val = -0.99712
// being shifted by : 
// -0.0341788

// Vertex x: -0.029124
// Vertex y: 0.067214
// Vertex z: 3.32855
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0286521
// Vertex y: 0.0666014
// Vertex z: 9.67302
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.031186
// Vertex y: 0.0847825
// Vertex z: 2.54682
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.017899
// Vertex y: 0.0635979
// Vertex z: 3.89679
// ---------------------------------
// ---------------------------------
// mva val = -0.99668
// being shifted by : 
// -0.0394023

// Vertex x: -0.0241867
// Vertex y: 0.0679888
// Vertex z: 0.00367349
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0238319
// Vertex y: 0.0774567
// Vertex z: -3.89929
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0222069
// Vertex y: 0.0725338
// Vertex z: -2.3325
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.02301
// Vertex y: 0.0710796
// Vertex z: 2.4042
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0351056
// Vertex y: 0.0578184
// Vertex z: -2.44508
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0248584
// Vertex y: 0.0751172
// Vertex z: 1.45528
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0304414
// Vertex y: 0.0691536
// Vertex z: -1.62724
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0335603
// Vertex y: 0.0702266
// Vertex z: -3.86238
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0131435
// Vertex y: 0.0691479
// Vertex z: -4.97994
// ---------------------------------
// ---------------------------------
// mva val = -0.99777
// being shifted by : 
// -0.0264571

// Vertex x: -0.0349087
// Vertex y: 0.0755583
// Vertex z: -0.741475
// ---------------------------------
// ---------------------------------
// mva val = -0.997729
// being shifted by : 
// -0.0269451

// Vertex x: -0.0466758
// Vertex y: 0.0696975
// Vertex z: 0.725605
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0312982
// Vertex y: 0.0684877
// Vertex z: 8.82161
// ---------------------------------
// ---------------------------------
// mva val = -0.997729
// being shifted by : 
// -0.0269451

// Vertex x: -0.025784
// Vertex y: 0.0715802
// Vertex z: 0.93881
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0330463
// Vertex y: 0.0546973
// Vertex z: 0.470302
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0250638
// Vertex y: 0.0719174
// Vertex z: -1.41643
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.028317
// Vertex y: 0.0661768
// Vertex z: 4.36176
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0175241
// Vertex y: 0.129554
// Vertex z: 3.60592
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.011143
// Vertex y: 0.0510278
// Vertex z: 5.95073
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0344007
// Vertex y: 0.0713908
// Vertex z: 7.17705
// ---------------------------------
// ---------------------------------
// mva val = 0.938357
// being shifted by : 
// 0.00476994

// Vertex x: -0.0245905
// Vertex y: 0.0705721
// Vertex z: -0.969548
// ---------------------------------
// ---------------------------------
// mva val = 0.951237
// being shifted by : 
// 0.00405803

// Vertex x: -0.0257838
// Vertex y: 0.068611
// Vertex z: 2.66782
// ---------------------------------
// ---------------------------------
// mva val = 0.853989
// being shifted by : 
// 0.011815

// Vertex x: -0.0257798
// Vertex y: 0.0703014
// Vertex z: 0.867136
// ---------------------------------
// ---------------------------------
// mva val = 0.926885
// being shifted by : 
// 0.00550291

// Vertex x: -0.0256227
// Vertex y: 0.0681952
// Vertex z: -5.82667
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0261578
// Vertex y: 0.0726024
// Vertex z: 3.14332
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0264752
// Vertex y: 0.0694427
// Vertex z: -2.8736
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.023153
// Vertex y: 0.0696622
// Vertex z: -0.691108
// ---------------------------------
// ---------------------------------
// mva val = 0.753042
// being shifted by : 
// 0.0224124

// Vertex x: -0.0240079
// Vertex y: 0.0708847
// Vertex z: -3.76259
// ---------------------------------
// ---------------------------------
// mva val = 0.951639
// being shifted by : 
// 0.00402784

// Vertex x: -0.0241331
// Vertex y: 0.068394
// Vertex z: 6.7068
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0278316
// Vertex y: 0.0708665
// Vertex z: 4.66578
// ---------------------------------
// ---------------------------------
// mva val = 0.941991
// being shifted by : 
// 0.00456135

// Vertex x: -0.0247293
// Vertex y: 0.0681341
// Vertex z: 0.0395433
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0266014
// Vertex y: 0.0709273
// Vertex z: 4.58677
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0256405
// Vertex y: 0.0690473
// Vertex z: 4.19585
// ---------------------------------
// ---------------------------------
// mva val = 0.941991
// being shifted by : 
// 0.00456135

// Vertex x: -0.0313374
// Vertex y: 0.0680999
// Vertex z: -0.038734
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.02695
// Vertex y: 0.0670679
// Vertex z: 4.235
// ---------------------------------
// ---------------------------------
// mva val = 0.951639
// being shifted by : 
// 0.00402784

// Vertex x: -0.0263528
// Vertex y: 0.063713
// Vertex z: 6.66545
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.029124
// Vertex y: 0.067214
// Vertex z: 3.32855
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0286521
// Vertex y: 0.0666014
// Vertex z: 9.67302
// ---------------------------------
// ---------------------------------
// mva val = 0.951237
// being shifted by : 
// 0.00405803

// Vertex x: -0.031186
// Vertex y: 0.0847825
// Vertex z: 2.54682
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.017899
// Vertex y: 0.0635979
// Vertex z: 3.89679
// ---------------------------------
// ---------------------------------
// mva val = 0.941991
// being shifted by : 
// 0.00456135

// Vertex x: -0.0241867
// Vertex y: 0.0679888
// Vertex z: 0.00367349
// ---------------------------------
// ---------------------------------
// mva val = 0.753042
// being shifted by : 
// 0.0224124

// Vertex x: -0.0238319
// Vertex y: 0.0774567
// Vertex z: -3.89929
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0222069
// Vertex y: 0.0725338
// Vertex z: -2.3325
// ---------------------------------
// ---------------------------------
// mva val = 0.951237
// being shifted by : 
// 0.00405803

// Vertex x: -0.02301
// Vertex y: 0.0710796
// Vertex z: 2.4042
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0351056
// Vertex y: 0.0578184
// Vertex z: -2.44508
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0248584
// Vertex y: 0.0751172
// Vertex z: 1.45528
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0304414
// Vertex y: 0.0691536
// Vertex z: -1.62724
// ---------------------------------
// ---------------------------------
// mva val = 0.753042
// being shifted by : 
// 0.0224124

// Vertex x: -0.0335603
// Vertex y: 0.0702266
// Vertex z: -3.86238
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0131435
// Vertex y: 0.0691479
// Vertex z: -4.97994
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0349087
// Vertex y: 0.0755583
// Vertex z: -0.741475
// ---------------------------------
// ---------------------------------
// mva val = 0.853989
// being shifted by : 
// 0.011815

// Vertex x: -0.0466758
// Vertex y: 0.0696975
// Vertex z: 0.725605
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0312982
// Vertex y: 0.0684877
// Vertex z: 8.82161
// ---------------------------------
// ---------------------------------
// mva val = 0.853989
// being shifted by : 
// 0.011815

// Vertex x: -0.025784
// Vertex y: 0.0715802
// Vertex z: 0.93881
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0330463
// Vertex y: 0.0546973
// Vertex z: 0.470302
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0250638
// Vertex y: 0.0719174
// Vertex z: -1.41643
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.028317
// Vertex y: 0.0661768
// Vertex z: 4.36176
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0175241
// Vertex y: 0.129554
// Vertex z: 3.60592
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.011143
// Vertex y: 0.0510278
// Vertex z: 5.95073
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// 0.0038665

// Vertex x: -0.0344007
// Vertex y: 0.0713908
// Vertex z: 7.17705
// ---------------------------------
// ---------------------------------
// mva val = -0.997835
// being shifted by : 
// 0.00208949

// Vertex x: -0.0245905
// Vertex y: 0.0705721
// Vertex z: -0.969548
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0257838
// Vertex y: 0.068611
// Vertex z: 2.66782
// ---------------------------------
// ---------------------------------
// mva val = -0.997729
// being shifted by : 
// 0.002191

// Vertex x: -0.0257798
// Vertex y: 0.0703014
// Vertex z: 0.867136
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0256227
// Vertex y: 0.0681952
// Vertex z: -5.82667
// ---------------------------------
// ---------------------------------
// mva val = -0.997778
// being shifted by : 
// 0.00214447

// Vertex x: -0.0261578
// Vertex y: 0.0726024
// Vertex z: 3.14332
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0264752
// Vertex y: 0.0694427
// Vertex z: -2.8736
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.023153
// Vertex y: 0.0696622
// Vertex z: -0.691108
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0240079
// Vertex y: 0.0708847
// Vertex z: -3.76259
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0241331
// Vertex y: 0.068394
// Vertex z: 6.7068
// ---------------------------------
// ---------------------------------
// mva val = -0.996986
// being shifted by : 
// 0.00290809

// Vertex x: -0.0278316
// Vertex y: 0.0708665
// Vertex z: 4.66578
// ---------------------------------
// ---------------------------------
// mva val = -0.99668
// being shifted by : 
// 0.00320394

// Vertex x: -0.0247293
// Vertex y: 0.0681341
// Vertex z: 0.0395433
// ---------------------------------
// ---------------------------------
// mva val = -0.996986
// being shifted by : 
// 0.00290809

// Vertex x: -0.0266014
// Vertex y: 0.0709273
// Vertex z: 4.58677
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0256405
// Vertex y: 0.0690473
// Vertex z: 4.19585
// ---------------------------------
// ---------------------------------
// mva val = -0.99668
// being shifted by : 
// 0.00320394

// Vertex x: -0.0313374
// Vertex y: 0.0680999
// Vertex z: -0.038734
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.02695
// Vertex y: 0.0670679
// Vertex z: 4.235
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0263528
// Vertex y: 0.063713
// Vertex z: 6.66545
// ---------------------------------
// ---------------------------------
// mva val = -0.99712
// being shifted by : 
// 0.0027792

// Vertex x: -0.029124
// Vertex y: 0.067214
// Vertex z: 3.32855
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0286521
// Vertex y: 0.0666014
// Vertex z: 9.67302
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.031186
// Vertex y: 0.0847825
// Vertex z: 2.54682
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.017899
// Vertex y: 0.0635979
// Vertex z: 3.89679
// ---------------------------------
// ---------------------------------
// mva val = -0.99668
// being shifted by : 
// 0.00320394

// Vertex x: -0.0241867
// Vertex y: 0.0679888
// Vertex z: 0.00367349
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0238319
// Vertex y: 0.0774567
// Vertex z: -3.89929
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0222069
// Vertex y: 0.0725338
// Vertex z: -2.3325
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.02301
// Vertex y: 0.0710796
// Vertex z: 2.4042
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0351056
// Vertex y: 0.0578184
// Vertex z: -2.44508
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0248584
// Vertex y: 0.0751172
// Vertex z: 1.45528
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0304414
// Vertex y: 0.0691536
// Vertex z: -1.62724
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0335603
// Vertex y: 0.0702266
// Vertex z: -3.86238
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0131435
// Vertex y: 0.0691479
// Vertex z: -4.97994
// ---------------------------------
// ---------------------------------
// mva val = -0.99777
// being shifted by : 
// 0.00215132

// Vertex x: -0.0349087
// Vertex y: 0.0755583
// Vertex z: -0.741475
// ---------------------------------
// ---------------------------------
// mva val = -0.997729
// being shifted by : 
// 0.002191

// Vertex x: -0.0466758
// Vertex y: 0.0696975
// Vertex z: 0.725605
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0312982
// Vertex y: 0.0684877
// Vertex z: 8.82161
// ---------------------------------
// ---------------------------------
// mva val = -0.997729
// being shifted by : 
// 0.002191

// Vertex x: -0.025784
// Vertex y: 0.0715802
// Vertex z: 0.93881
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0330463
// Vertex y: 0.0546973
// Vertex z: 0.470302
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0250638
// Vertex y: 0.0719174
// Vertex z: -1.41643
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.028317
// Vertex y: 0.0661768
// Vertex z: 4.36176
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0175241
// Vertex y: 0.129554
// Vertex z: 3.60592
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.011143
// Vertex y: 0.0510278
// Vertex z: 5.95073
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// 0.0035672

// Vertex x: -0.0344007
// Vertex y: 0.0713908
// Vertex z: 7.17705
// ---------------------------------
// ---------------------------------
// mva val = 0.938357
// being shifted by : 
// -0.00467031

// Vertex x: -0.0245905
// Vertex y: 0.0705721
// Vertex z: -0.969548
// ---------------------------------
// ---------------------------------
// mva val = 0.951237
// being shifted by : 
// -0.00403761

// Vertex x: -0.0257838
// Vertex y: 0.068611
// Vertex z: 2.66782
// ---------------------------------
// ---------------------------------
// mva val = 0.853989
// being shifted by : 
// -0.0108398

// Vertex x: -0.0257798
// Vertex y: 0.0703014
// Vertex z: 0.867136
// ---------------------------------
// ---------------------------------
// mva val = 0.926885
// being shifted by : 
// -0.00530599

// Vertex x: -0.0256227
// Vertex y: 0.0681952
// Vertex z: -5.82667
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0261578
// Vertex y: 0.0726024
// Vertex z: 3.14332
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0264752
// Vertex y: 0.0694427
// Vertex z: -2.8736
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.023153
// Vertex y: 0.0696622
// Vertex z: -0.691108
// ---------------------------------
// ---------------------------------
// mva val = 0.753042
// being shifted by : 
// -0.0200264

// Vertex x: -0.0240079
// Vertex y: 0.0708847
// Vertex z: -3.76259
// ---------------------------------
// ---------------------------------
// mva val = 0.951639
// being shifted by : 
// -0.00402308

// Vertex x: -0.0241331
// Vertex y: 0.068394
// Vertex z: 6.7068
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0278316
// Vertex y: 0.0708665
// Vertex z: 4.66578
// ---------------------------------
// ---------------------------------
// mva val = 0.941991
// being shifted by : 
// -0.00448821

// Vertex x: -0.0247293
// Vertex y: 0.0681341
// Vertex z: 0.0395433
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0266014
// Vertex y: 0.0709273
// Vertex z: 4.58677
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0256405
// Vertex y: 0.0690473
// Vertex z: 4.19585
// ---------------------------------
// ---------------------------------
// mva val = 0.941991
// being shifted by : 
// -0.00448821

// Vertex x: -0.0313374
// Vertex y: 0.0680999
// Vertex z: -0.038734
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.02695
// Vertex y: 0.0670679
// Vertex z: 4.235
// ---------------------------------
// ---------------------------------
// mva val = 0.951639
// being shifted by : 
// -0.00402308

// Vertex x: -0.0263528
// Vertex y: 0.063713
// Vertex z: 6.66545
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.029124
// Vertex y: 0.067214
// Vertex z: 3.32855
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0286521
// Vertex y: 0.0666014
// Vertex z: 9.67302
// ---------------------------------
// ---------------------------------
// mva val = 0.951237
// being shifted by : 
// -0.00403761

// Vertex x: -0.031186
// Vertex y: 0.0847825
// Vertex z: 2.54682
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.017899
// Vertex y: 0.0635979
// Vertex z: 3.89679
// ---------------------------------
// ---------------------------------
// mva val = 0.941991
// being shifted by : 
// -0.00448821

// Vertex x: -0.0241867
// Vertex y: 0.0679888
// Vertex z: 0.00367349
// ---------------------------------
// ---------------------------------
// mva val = 0.753042
// being shifted by : 
// -0.0200264

// Vertex x: -0.0238319
// Vertex y: 0.0774567
// Vertex z: -3.89929
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0222069
// Vertex y: 0.0725338
// Vertex z: -2.3325
// ---------------------------------
// ---------------------------------
// mva val = 0.951237
// being shifted by : 
// -0.00403761

// Vertex x: -0.02301
// Vertex y: 0.0710796
// Vertex z: 2.4042
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0351056
// Vertex y: 0.0578184
// Vertex z: -2.44508
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0248584
// Vertex y: 0.0751172
// Vertex z: 1.45528
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0304414
// Vertex y: 0.0691536
// Vertex z: -1.62724
// ---------------------------------
// ---------------------------------
// mva val = 0.753042
// being shifted by : 
// -0.0200264

// Vertex x: -0.0335603
// Vertex y: 0.0702266
// Vertex z: -3.86238
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0131435
// Vertex y: 0.0691479
// Vertex z: -4.97994
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0349087
// Vertex y: 0.0755583
// Vertex z: -0.741475
// ---------------------------------
// ---------------------------------
// mva val = 0.853989
// being shifted by : 
// -0.0108398

// Vertex x: -0.0466758
// Vertex y: 0.0696975
// Vertex z: 0.725605
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0312982
// Vertex y: 0.0684877
// Vertex z: 8.82161
// ---------------------------------
// ---------------------------------
// mva val = 0.853989
// being shifted by : 
// -0.0108398

// Vertex x: -0.025784
// Vertex y: 0.0715802
// Vertex z: 0.93881
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0330463
// Vertex y: 0.0546973
// Vertex z: 0.470302
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0250638
// Vertex y: 0.0719174
// Vertex z: -1.41643
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.028317
// Vertex y: 0.0661768
// Vertex z: 4.36176
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0175241
// Vertex y: 0.129554
// Vertex z: 3.60592
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.011143
// Vertex y: 0.0510278
// Vertex z: 5.95073
// ---------------------------------
// ---------------------------------
// mva val = 0.955119
// being shifted by : 
// -0.00388343

// Vertex x: -0.0344007
// Vertex y: 0.0713908
// Vertex z: 7.17705
// ---------------------------------
// ---------------------------------
// mva val = -0.997835
// being shifted by : 
// -0.0256967

// Vertex x: -0.0245905
// Vertex y: 0.0705721
// Vertex z: -0.969548
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0257838
// Vertex y: 0.068611
// Vertex z: 2.66782
// ---------------------------------
// ---------------------------------
// mva val = -0.997729
// being shifted by : 
// -0.0269451

// Vertex x: -0.0257798
// Vertex y: 0.0703014
// Vertex z: 0.867136
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0256227
// Vertex y: 0.0681952
// Vertex z: -5.82667
// ---------------------------------
// ---------------------------------
// mva val = -0.997778
// being shifted by : 
// -0.0263729

// Vertex x: -0.0261578
// Vertex y: 0.0726024
// Vertex z: 3.14332
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0264752
// Vertex y: 0.0694427
// Vertex z: -2.8736
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.023153
// Vertex y: 0.0696622
// Vertex z: -0.691108
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0240079
// Vertex y: 0.0708847
// Vertex z: -3.76259
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0241331
// Vertex y: 0.068394
// Vertex z: 6.7068
// ---------------------------------
// ---------------------------------
// mva val = -0.996986
// being shifted by : 
// -0.0357639

// Vertex x: -0.0278316
// Vertex y: 0.0708665
// Vertex z: 4.66578
// ---------------------------------
// ---------------------------------
// mva val = -0.99668
// being shifted by : 
// -0.0394023

// Vertex x: -0.0247293
// Vertex y: 0.0681341
// Vertex z: 0.0395433
// ---------------------------------
// ---------------------------------
// mva val = -0.996986
// being shifted by : 
// -0.0357639

// Vertex x: -0.0266014
// Vertex y: 0.0709273
// Vertex z: 4.58677
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0256405
// Vertex y: 0.0690473
// Vertex z: 4.19585
// ---------------------------------
// ---------------------------------
// mva val = -0.99668
// being shifted by : 
// -0.0394023

// Vertex x: -0.0313374
// Vertex y: 0.0680999
// Vertex z: -0.038734
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.02695
// Vertex y: 0.0670679
// Vertex z: 4.235
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0263528
// Vertex y: 0.063713
// Vertex z: 6.66545
// ---------------------------------
// ---------------------------------
// mva val = -0.99712
// being shifted by : 
// -0.0341788

// Vertex x: -0.029124
// Vertex y: 0.067214
// Vertex z: 3.32855
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0286521
// Vertex y: 0.0666014
// Vertex z: 9.67302
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.031186
// Vertex y: 0.0847825
// Vertex z: 2.54682
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.017899
// Vertex y: 0.0635979
// Vertex z: 3.89679
// ---------------------------------
// ---------------------------------
// mva val = -0.99668
// being shifted by : 
// -0.0394023

// Vertex x: -0.0241867
// Vertex y: 0.0679888
// Vertex z: 0.00367349
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0238319
// Vertex y: 0.0774567
// Vertex z: -3.89929
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0222069
// Vertex y: 0.0725338
// Vertex z: -2.3325
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.02301
// Vertex y: 0.0710796
// Vertex z: 2.4042
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0351056
// Vertex y: 0.0578184
// Vertex z: -2.44508
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0248584
// Vertex y: 0.0751172
// Vertex z: 1.45528
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0304414
// Vertex y: 0.0691536
// Vertex z: -1.62724
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0335603
// Vertex y: 0.0702266
// Vertex z: -3.86238
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0131435
// Vertex y: 0.0691479
// Vertex z: -4.97994
// ---------------------------------
// ---------------------------------
// mva val = -0.99777
// being shifted by : 
// -0.0264571

// Vertex x: -0.0349087
// Vertex y: 0.0755583
// Vertex z: -0.741475
// ---------------------------------
// ---------------------------------
// mva val = -0.997729
// being shifted by : 
// -0.0269451

// Vertex x: -0.0466758
// Vertex y: 0.0696975
// Vertex z: 0.725605
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0312982
// Vertex y: 0.0684877
// Vertex z: 8.82161
// ---------------------------------
// ---------------------------------
// mva val = -0.997729
// being shifted by : 
// -0.0269451

// Vertex x: -0.025784
// Vertex y: 0.0715802
// Vertex z: 0.93881
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0330463
// Vertex y: 0.0546973
// Vertex z: 0.470302
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0250638
// Vertex y: 0.0719174
// Vertex z: -1.41643
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.028317
// Vertex y: 0.0661768
// Vertex z: 4.36176
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0175241
// Vertex y: 0.129554
// Vertex z: 3.60592
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.011143
// Vertex y: 0.0510278
// Vertex z: 5.95073
// ---------------------------------
// ---------------------------------
// mva val = -0.996303
// being shifted by : 
// -0.0438697

// Vertex x: -0.0344007
// Vertex y: 0.0713908
// Vertex z: 7.17705
// ---------------------------------
// diphoton vertex index = 0
// diphoton vertex x = -0.0245905
// diphoton vertex y = 0.0705721
// diphoton vertex z = -0.969548
// diphoton vertex index = 0
// diphoton vertex x = -0.0245905
// diphoton vertex y = 0.0705721
// diphoton vertex z = -0.969548
