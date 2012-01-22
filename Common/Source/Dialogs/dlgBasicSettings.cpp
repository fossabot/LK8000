/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights

   $Id: dlgBasicSettings.cpp,v 1.1 2011/12/21 10:29:29 root Exp root $
*/

#include "externs.h"
#include "McReady.h"
#include "Atmosphere.h"
#include "dlgTools.h"
#include "InfoBoxLayout.h"


extern HWND   hWndMainWindow;
static WndForm *wf=NULL;


// static bool BallastTimerActive = false;

static void OnCloseClicked(WindowControl * Sender){
(void)Sender;
	wf->SetModalResult(mrOK);
}

static void OnBallastDump(WindowControl *Sender){
(void)Sender;
        BallastTimerActive=!BallastTimerActive;
	wf->SetModalResult(mrOK);
}


static double INHg=0;

static void OnQnhData(DataField *Sender, DataField::DataAccessKind_t Mode){
  WndProperty* wp;

  switch(Mode){
	case DataField::daGet:
		if (PressureHg) {
			INHg=QNH/TOHPA;
			Sender->Set(INHg); 
		} else {
			Sender->Set(QNH); 
		}
		break;
	case DataField::daPut: 
	case DataField::daChange:
		if (PressureHg) {
			INHg = Sender->GetAsFloat();
			QNH=INHg*TOHPA;
		} else {
			QNH = Sender->GetAsFloat();
		}
		if (CALCULATED_INFO.Flying) QNH=fabs(QNH); 
		devPutQNH(devAll(), QNH);
		CAirspaceManager::Instance().QnhChangeNotify(QNH);
		// VarioWriteSettings();

		wp = (WndProperty*)wf->FindByName(TEXT("prpAltitude"));
		if (wp) {
			wp->GetDataField()->
			SetAsFloat(Units::ToUserAltitude(GPS_INFO.BaroAltitude));
			wp->RefreshDisplay();
		}
		break;
	default:
		break;
  }

}


static void OnAltitudeData(DataField *Sender, DataField::DataAccessKind_t Mode){
  static double newalt=0;
  WndProperty* wp;
  switch(Mode){
	case DataField::daGet:
	break;
  case DataField::daPut:
  case DataField::daChange:
	newalt = Sender->GetAsFloat();
        QNH=FindQNH(GPS_INFO.BaroAltitude,Units::ToSysAltitude(newalt));  // 100411
	wp = (WndProperty*)wf->FindByName(TEXT("prpQNH"));
	if (wp) {
		if (PressureHg) {
			INHg=QNH/TOHPA;
			wp->GetDataField()-> SetAsFloat(INHg);
		} else {
			wp->GetDataField()-> SetAsFloat(QNH);
		}
		wp->RefreshDisplay();
	}
    CAirspaceManager::Instance().QnhChangeNotify(QNH);
	break;
  case DataField::daInc:
  case DataField::daDec:
  case DataField::daSpecial:
    break;
  }
}


static void SetBallast(bool updateDevices) {
  WndProperty* wp;

  GlidePolar::SetBallast();
  if (updateDevices) { 
	devPutBallast(devA(), BALLAST);
	devPutBallast(devB(), BALLAST);
  }
	static double foldBallast = BALLAST;
	if(fabs (foldBallast - BALLAST) > 0.01) /* update on change only */
	{
  wp = (WndProperty*)wf->FindByName(TEXT("prpBallastPercent"));
      if (wp)
      {
    wp->GetDataField()->Set(BALLAST*100);
    wp->RefreshDisplay();
  	    foldBallast = BALLAST;
  }
	}

	static double foldLiter = GlidePolar::BallastLitres;
	if(fabs (foldLiter-GlidePolar::BallastLitres ) > 0.01) /* update on change only */
	{
  wp = (WndProperty*)wf->FindByName(TEXT("prpBallastLitres"));
	  if (wp)
	  {
	    wp->GetDataField()->SetAsFloat(GlidePolar::BallastLitres);
    wp->RefreshDisplay();
		foldLiter = GlidePolar::BallastLitres ;
  }
	}

	static double fOldLoad = GlidePolar::WingLoading;
	if(fabs (fOldLoad-GlidePolar::WingLoading ) > 0.01) /* update on change only */
	{
  wp = (WndProperty*)wf->FindByName(TEXT("prpWingLoading"));
  if (wp) {
    wp->GetDataField()-> SetAsFloat(GlidePolar::WingLoading);
    wp->RefreshDisplay();
		fOldLoad = GlidePolar::WingLoading;
	  }
  }
  // SetFocus( ((WndButton *)wf->FindByName(TEXT("buttonClose")))->GetHandle()); // not needed
}

//int BallastSecsToEmpty = 120;

static int OnTimerNotify(WindowControl * Sender) {
  (void)Sender;
  // devices are updates by BallastDump() method when dumping water ballast
  SetBallast(false); 

  WndProperty* wp;
  static double altlast = GPS_INFO.BaroAltitude;
  if (fabs(GPS_INFO.BaroAltitude-altlast)>1) {
    wp = (WndProperty*)wf->FindByName(TEXT("prpAltitude"));
    if (wp) {
      wp->GetDataField()->
	SetAsFloat(Units::ToUserAltitude(GPS_INFO.BaroAltitude));
      wp->RefreshDisplay();
    }
  }
  altlast = GPS_INFO.BaroAltitude;

static float  flastBugs=BUGS;
  if (fabs(flastBugs-BUGS) >= 0.001) /* update on change only */
  {
    if(wf)
    {
      wp = (WndProperty*)wf->FindByName(gettext(TEXT("prpBugs")));
	  if (wp)
	  {
		wp->GetDataField()->SetAsFloat(BUGS*100);
		wp->RefreshDisplay();
		flastBugs = BUGS;
	  }
    }
  }

  return 0;
}


static void OnBallastData(DataField *Sender, DataField::DataAccessKind_t Mode){
  static double lastRead = -1;

  switch(Mode){
  case DataField::daSpecial:
    if (BALLAST>0.01) {
      BallastTimerActive = !BallastTimerActive;
    } else {
      BallastTimerActive = false;
    }
    ((WndButton *)wf->FindByName(TEXT("buttonDumpBallast")))->SetVisible(!BallastTimerActive);
    ((WndButton *)wf->FindByName(TEXT("buttonStopDump")))->SetVisible(BallastTimerActive);
    break;
  case DataField::daGet:
    lastRead = BALLAST;
    Sender->Set(BALLAST*100);
    break;
  case DataField::daChange:
  case DataField::daPut:
    if (fabs(lastRead-Sender->GetAsFloat()/100.0) >= 0.005){
      lastRead = BALLAST = Sender->GetAsFloat()/100.0;
      SetBallast(true);
    }
    break;
  case DataField::daInc:
  case DataField::daDec:
    break;
  }
}

static void OnBugsData(DataField *Sender, DataField::DataAccessKind_t Mode){

  static double lastRead = -1;

  switch(Mode){
    case DataField::daGet:
      lastRead = BUGS;
      Sender->Set(BUGS*100);
    break;
    case DataField::daChange:
    case DataField::daPut:
      if (fabs(lastRead-Sender->GetAsFloat()/100.0) >= 0.005)
      {
        lastRead = BUGS = Sender->GetAsFloat()/100.0;
        GlidePolar::SetBallast();
        devPutBugs(devA(), BUGS);
        devPutBugs(devB(), BUGS);
      }
    break;
  case DataField::daInc:
  case DataField::daDec:
  case DataField::daSpecial:
    break;
  }

}

static void OnWingLoadingData(DataField *Sender, DataField::DataAccessKind_t Mode) { // 100127
  static double lastRead = -1;
  switch(Mode){
	case DataField::daGet:
		lastRead = GlidePolar::WingLoading;
		Sender->Set(GlidePolar::WingLoading);
		break;
	case DataField::daChange:
	case DataField::daPut:
		if (fabs(lastRead-Sender->GetAsFloat()) > 0) {
			lastRead = Sender->GetAsFloat();
			WeightOffset(lastRead);
		}
		break;
  case DataField::daInc:
  case DataField::daDec:
  case DataField::daSpecial:
    break;
  }
}


static void OnTempData(DataField *Sender, DataField::DataAccessKind_t Mode){
  static double lastRead = -1;

  switch(Mode){
    case DataField::daGet:
      lastRead = CuSonde::maxGroundTemperature;
      Sender->Set(CuSonde::maxGroundTemperature);
    break;
    case DataField::daChange:
    case DataField::daPut:
      if (fabs(lastRead-Sender->GetAsFloat()) >= 1.0){
        lastRead = Sender->GetAsFloat();
        CuSonde::setForecastTemperature(Sender->GetAsFloat());
      }
    break;
  case DataField::daInc:
  case DataField::daDec:
  case DataField::daSpecial:
    break;
  }
}


static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnBugsData),
  DeclareCallBackEntry(OnWingLoadingData), 
  DeclareCallBackEntry(OnTempData),
  DeclareCallBackEntry(OnBallastData),
  DeclareCallBackEntry(OnAltitudeData),
  DeclareCallBackEntry(OnQnhData), 
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnBallastDump),
  DeclareCallBackEntry(NULL)
};


void dlgBasicSettingsShowModal(void){


  char filename[MAX_PATH];
  if (!ScreenLandscape) {
	LocalPathS(filename, TEXT("dlgBasicSettings_L.xml"));
	wf = dlgLoadFromXML(CallBackTable, filename, hWndMainWindow, TEXT("IDR_XML_BASICSETTINGS_L"));
  } else {
	LocalPathS(filename, TEXT("dlgBasicSettings.xml"));
	wf = dlgLoadFromXML(CallBackTable, filename, hWndMainWindow, TEXT("IDR_XML_BASICSETTINGS"));
  }

  WndProperty* wp;

//  BallastTimerActive = false;

  if (wf) {

    wf->SetTimerNotify(OnTimerNotify);

    ((WndButton *)wf->FindByName(TEXT("buttonDumpBallast")))->SetVisible(!BallastTimerActive);
    ((WndButton *)wf->FindByName(TEXT("buttonStopDump")))->SetVisible(BallastTimerActive);

    wp = (WndProperty*)wf->FindByName(TEXT("prpAltitude"));
    if (wp) {
      wp->GetDataField()->SetAsFloat(
	       Units::ToUserAltitude(GPS_INFO.BaroAltitude));
      wp->GetDataField()->SetUnits(Units::GetAltitudeName());
      wp->RefreshDisplay();
    }

    wp = (WndProperty*)wf->FindByName(TEXT("prpBallastPercent"));
    if (wp) {
      if (WEIGHTS[2]==0) wp->SetReadOnly(1);
      wp->RefreshDisplay();
    }

    wp = (WndProperty*)wf->FindByName(TEXT("prpBallastLitres"));
    if (wp) {
      if (WEIGHTS[2]>0) {
        wp->GetDataField()-> SetAsFloat(GlidePolar::BallastLitres);
      } else {
	wp->SetReadOnly(1);
      }
      wp->RefreshDisplay();
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpWingLoading"));
    if (wp) {
      if (GlidePolar::WingLoading>0.1) {
	wp->GetDataField()-> SetAsFloat(GlidePolar::WingLoading);
      } else {
	wp->SetVisible(false);
      }
      wp->RefreshDisplay();
    }
    if (CALCULATED_INFO.Flying) {
	wp = (WndProperty*)wf->FindByName(TEXT("prpQNH"));
	if (wp) {
		wp->GetDataField()->SetDisplayFormat(_T("%.0f"));
	}
    }

    wf->ShowModal();
    delete wf;
  }
  wf = NULL;

}

