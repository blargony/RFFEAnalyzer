#include "BISSAnalyzer.h"
#include "BISSAnalyzerSettings.h"
#include <AnalyzerChannelData.h>

BISSAnalyzer::BISSAnalyzer()
:	Analyzer2(),
	mSettings( new BISSAnalyzerSettings() ),
	mSimulationInitilized( false )

{
	SetAnalyzerSettings( mSettings.get() );
	
	m_counter_cdm = 0;
	m_counter_cds = 0;
	m_counter_newframe = 0;
	mul_cdmframe_index = 0;
	mul_cdsframe_index = 0;
}

BISSAnalyzer::~BISSAnalyzer()
{
	KillThread();
}

//---------------------------------------------------------------------------------------------------------------------------------
void BISSAnalyzer::WorkerThread()
//---------------------------------------------------------------------------------------------------------------------------------
{
	m_counter_cdm = 0;
	m_counter_cds = 0;
	m_counter_newframe = 0;
	mul_cdmframe_index = 0;
	mul_cdsframe_index = 0;



	mSampleRateHz = GetSampleRate();
	Trigger = GetTriggerSample();

	mMa = GetAnalyzerChannelData( mSettings->mMaChannel );
	mSlo = GetAnalyzerChannelData( mSettings->mSloChannel );


	for( ; ; )
		{		
			GetData();
			CheckIfThreadShouldExit();
		}
}
//---------------------------------------------------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------------------------------------------------
void BISSAnalyzer::FindStartBit()
//---------------------------------------------------------------------------------------------------------------------------------
{
	mMa->AdvanceToNextEdge();

	if ( mMa->GetBitState() == BIT_HIGH)
		mMa->AdvanceToNextEdge();

	//m_startperiod = mMa->GetSampleNumber(); //Startsamplenummer für 0,5 Perioden --> m_endperiod
	//mResults->AddMarker( mMa->GetSampleNumber(), AnalyzerResults::Start, mSettings->mMaChannel );
	//m_endperiod = mMa->GetSampleOfNextEdge();// Endsamplenummer fü 0,5 Perioden --> m_startperiod		
	//mResults->AddMarker( mMa->GetSampleOfNextEdge(), AnalyzerResults::Start, mSettings->mMaChannel );
	//m_samplesperperiod = (int)((m_endperiod - m_startperiod)*3);//Berechnung Samples für 1,5 Perioden

	mSlo->AdvanceToAbsPosition( mMa->GetSampleNumber() );

	while ( mSlo->GetBitState() == BIT_HIGH )
	{
		mMa->AdvanceToNextEdge();
		mSlo->AdvanceToAbsPosition( mMa->GetSampleNumber() );
	}
	while ( mSlo->GetBitState() == BIT_LOW )
	{
		mMa->AdvanceToNextEdge();
		mSlo->AdvanceToAbsPosition( mMa->GetSampleNumber() );
	}

	switch ( mMa->GetBitState() )
	{
	case BIT_HIGH:
		mMa->AdvanceToNextEdge();
		break;

	case BIT_LOW:
		//Startflanke ist schon erreicht, nicht weiterspringen
		break;
	}
	m_startperiod = mMa->GetSampleNumber(); //Startsamplenummer 1,5 Perioden --> m_endperiod

	if (mSettings->mDatenart == 0)//Benutzerauswahl Registerdaten
	{
		mResults->AddMarker( mMa->GetSampleNumber(), AnalyzerResults::Start, mSettings->mMaChannel );
	}
	else
		mResults->AddMarker( mMa->GetSampleNumber(), AnalyzerResults::ErrorDot, mSettings->mMaChannel );
	
}
//---------------------------------------------------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------------------------------------------------
void BISSAnalyzer::GetData()
//---------------------------------------------------------------------------------------------------------------------------------
{
	FindStartBit();
		
	U64 ewcrc_daten;
	DataBuilder obj_ewcrc;
	
	U64 sensor_daten;
	DataBuilder obj_sensordaten;
	
	BitState bit_state_slo;
	BitState bit_state_ma;
	U64 ma_edge;
	U64 ma_edge_next;

	obj_sensordaten.Reset( &sensor_daten, AnalyzerEnums::MsbFirst, mSettings->mDataLength );
	
	int letzte_flanke ;
	letzte_flanke = ( mSettings->mDataLength + 8 );
	

	for( U32 i=0; i<=letzte_flanke; i++ )
	{
		mMa->AdvanceToNextEdge();//pos. Flanke
		mMa->AdvanceToNextEdge();//neg. Flanke

		bit_state_ma = mMa->GetBitState();   

		ma_edge = mMa->GetSampleNumber();
		ma_edge_next = mMa->GetSampleOfNextEdge();

		if ( bit_state_ma == BIT_LOW )
		{
			mSlo->AdvanceToAbsPosition( ma_edge );

			m_sdata_array[i] = mSlo->GetBitState();
			m_sdata_samplenummer_array[i] = mSlo->GetSampleNumber();

			if( i == 0 )//CDS erfassen starten, Ende einer Periode erfassen
			{
				m_endperiod = mMa->GetSampleNumber();// Endsamplenummer einer Periode --> m_startperiod		
				m_samplesperperiod = (int)((m_endperiod - m_startperiod)*1.5);//Berechnung Samples pro Periode * 1,5

				if (mSettings->mDatenart == 0)//Benutzerauswahl Registerdaten
				{									
					i = (letzte_flanke + 1);//Daten sammeln nach erfassen von CDS beenden

					GetCds( mSlo->GetBitState() );					
				}				
			}

			if( i == 1 )// erste Flanke Daten
			{
				mResults->AddMarker( ma_edge, AnalyzerResults::Start, mSettings->mMaChannel );
			}
			else if( i == mSettings->mDataLength )// lezte Flanke Daten 
			{
				mResults->AddMarker( ma_edge, AnalyzerResults::Stop, mSettings->mMaChannel );
			}
			else if ( i == (mSettings->mDataLength + 1) )// Start Flanke Error, Warning, CRC
			{
				mResults->AddMarker( ma_edge, AnalyzerResults::Start, mSettings->mMaChannel );
			}		
			else if ( i == (mSettings->mDataLength + 8) )// letzte Flanke Error, Warning, CRC
			{
				mResults->AddMarker( ma_edge, AnalyzerResults::Stop, mSettings->mMaChannel );
				
				//-------------------------------------------------------------------------------------------

				int j = 0;

				for (int j = 1; j <= mSettings->mDataLength; j++)//Daten
				{
					obj_sensordaten.AddBit( m_sdata_array[j]);
				}

				Frame SDATA_frame;
				SDATA_frame.mType = 1;
				SDATA_frame.mStartingSampleInclusive = m_sdata_samplenummer_array[1];
				SDATA_frame.mEndingSampleInclusive = m_sdata_samplenummer_array[mSettings->mDataLength];
				SDATA_frame.mData1 = sensor_daten;	
				SDATA_frame.mData2 = mSettings->mDataLength;
				SDATA_frame.mFlags = 1;
				mResults->AddFrame( SDATA_frame );
				mResults->CommitResults();

				//-------------------------------------------------------------------------------------------

				obj_ewcrc.Reset( &ewcrc_daten, AnalyzerEnums::MsbFirst, 2);

				for (j = (mSettings->mDataLength+1); j <= (mSettings->mDataLength+2); j++)//Error Warning
				{
					obj_ewcrc.AddBit( m_sdata_array[j]);
				}

				Frame EW_frame;
				EW_frame.mType = 1;
				EW_frame.mStartingSampleInclusive = m_sdata_samplenummer_array[(mSettings->mDataLength+1)];
				EW_frame.mEndingSampleInclusive = m_sdata_samplenummer_array[(mSettings->mDataLength+2)];
				EW_frame.mData1 = ewcrc_daten;
				EW_frame.mData2 = 2;
				EW_frame.mFlags = 2;
				mResults->AddFrame( EW_frame );
				mResults->CommitResults();

				//------------------------------------------------------------------------------------------

				obj_ewcrc.Reset( &ewcrc_daten, AnalyzerEnums::MsbFirst, 6);

				for (j = (mSettings->mDataLength+3); j <= (mSettings->mDataLength+8); j++)//CRC
				{
					obj_ewcrc.AddBit( m_sdata_array[j]);
				}

				Frame CRC_frame;
				CRC_frame.mType = 1;
				CRC_frame.mStartingSampleInclusive = m_sdata_samplenummer_array[(mSettings->mDataLength+3)];
				CRC_frame.mEndingSampleInclusive = m_sdata_samplenummer_array[(mSettings->mDataLength+8)];
				CRC_frame.mData1 = ewcrc_daten;
				CRC_frame.mData2 = 6;
				CRC_frame.mFlags = 3;
				mResults->AddFrame( CRC_frame );
				mResults->CommitResults();

				//------------------------------------------------------------------------------------------
			} 
			else if ( i > 0 ) 
			{
				mResults->AddMarker( mMa->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mMaChannel ); 
			}
		}
	}	
	
	while((ma_edge_next - ma_edge) < m_samplesperperiod )//Flanken überspringen bis neuer Frame beginnt
	{
		mMa->AdvanceToNextEdge();
		ma_edge = mMa->GetSampleNumber();
		ma_edge_next = mMa->GetSampleOfNextEdge();		
	}

	if ((ma_edge_next - ma_edge) > m_samplesperperiod )//CDM erkannt (Abfrage im Prinzip nicht erforderlich)
	{
		if (mSettings->mDatenart == 0)//Benutzerauswahl Registerdaten
		{
			GetCdm( mMa->GetBitState() );
		}
	}	
}
//---------------------------------------------------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------------------------------------------------
void BISSAnalyzer::GetCdm( BitState bit_state_cdm )
//---------------------------------------------------------------------------------------------------------------------------------
{
	m_counter_cdm ++;

	switch (bit_state_cdm)
	{
	case BIT_HIGH://0
					m_cdm_array[m_counter_cdm] = BIT_LOW;	
					m_cdm_samplenummer_array[m_counter_cdm] = mMa->GetSampleNumber();
					m_counter_newframe ++;
					mResults->AddMarker( mMa->GetSampleNumber(), AnalyzerResults::Zero, mSettings->mMaChannel );

					if (m_counter_newframe == 14)
					{	
						BuiltCdsFrames();
						BuiltCdmFrames();
						ShowCdmCds();
						m_counter_cdm = 0;
						mul_cdmframe_index = 0;
						mul_cdsframe_index = 0;
					}
					
					if (m_counter_newframe > 14)
					{
						m_counter_cdm = 0;
					}
					break;

	case BIT_LOW://1
					m_cdm_array[m_counter_cdm] = BIT_HIGH;
					m_cdm_samplenummer_array[m_counter_cdm] = mMa->GetSampleNumber();
					mResults->AddMarker( mMa->GetSampleNumber(), AnalyzerResults::One, mSettings->mMaChannel );
					m_counter_newframe = 0;
					break;
	}	
}
//---------------------------------------------------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------------------------------------------------
void BISSAnalyzer::GetCds( BitState bit_state_cds )
//---------------------------------------------------------------------------------------------------------------------------------
{
	switch (bit_state_cds)
	{
	case BIT_HIGH://1
					m_cds_array[m_counter_cds] = BIT_HIGH;	
					m_cds_samplenummer_array[m_counter_cds] = mMa->GetSampleNumber();
					mResults->AddMarker( mMa->GetSampleNumber(), AnalyzerResults::One, mSettings->mSloChannel );
					break;

	case BIT_LOW://0
					m_cds_array[m_counter_cds] = BIT_LOW;	
					m_cds_samplenummer_array[m_counter_cds] = mMa->GetSampleNumber();
					mResults->AddMarker( mMa->GetSampleNumber(), AnalyzerResults::Zero, mSettings->mSloChannel );
					break;
	}

	if (m_counter_newframe == 14)
	{
		//BuiltCdsFrames();
		m_counter_cds = 0;
	}

	if (m_counter_newframe > 14)
	{
		m_counter_cds = 0;
	}

	m_counter_cds ++;
}
//---------------------------------------------------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------------------------------------------------
void BISSAnalyzer::BuiltCdmFrames()
//---------------------------------------------------------------------------------------------------------------------------------
{
	unsigned long ulCDM = 1;

	if ((m_cdm_array[1] == BIT_HIGH) && (m_cdm_array[2] == BIT_HIGH))//Datenanzeige nur bei Start,CTS = 1
	{
		int j = 0;

		m_cdm.Reset( &m_cdm_daten, AnalyzerEnums::MsbFirst, 2);

		for (j = 1; j <= 2; j++)//Start, CTS
		{
			m_cdm.AddBit( m_cdm_array[j]);
		}

		Frame S_CTS_frame;
		S_CTS_frame.mType = 2;//Index CDM
		S_CTS_frame.mStartingSampleInclusive = m_cdm_samplenummer_array[1];
		S_CTS_frame.mEndingSampleInclusive = m_cdm_samplenummer_array[2];
		S_CTS_frame.mData1 = m_cdm_daten;	
		S_CTS_frame.mData2 = 2;//Index für die Anzahl der Bitstellen
		S_CTS_frame.mFlags = 1;//Index für die Textausgabe in BISSAnalyzerResults
		//mResults->AddChannelBubblesWillAppearOn (mSettings->mMaChannel);						
		//mResults->AddFrame( S_CTS_frame );
		//mResults->CommitResults();						
		AddMyFrame(S_CTS_frame,ulCDM);
		//---------------------------------------------------------------------------
		m_cdm.Reset( &m_cdm_daten, AnalyzerEnums::MsbFirst, 3);

		for (j = 3; j <= 5; j++)//ID
		{
			m_cdm.AddBit( m_cdm_array[j]);
		}

		Frame ID_frame;
		ID_frame.mType = 2;//Index CDM
		ID_frame.mStartingSampleInclusive = m_cdm_samplenummer_array[3];
		ID_frame.mEndingSampleInclusive = m_cdm_samplenummer_array[5];
		ID_frame.mData1 = m_cdm_daten;
		ID_frame.mData2 = 3;//Index für die Anzahl der Bitstellen
		ID_frame.mFlags = 2;//Index für die Textausgabe in BISSAnalyzerResults
		//mResults->AddChannelBubblesWillAppearOn (mSettings->mMaChannel);						
		//mResults->AddFrame( ID_frame );
		//mResults->CommitResults();	
		AddMyFrame(ID_frame,ulCDM);
		//--------------------------------------------------------------------------
		m_cdm.Reset( &m_cdm_daten, AnalyzerEnums::MsbFirst, 7);

		for (j = 6; j <= 12; j++)//Adresse
		{
			m_cdm.AddBit( m_cdm_array[j]);
		}

		Frame ADR_CDM_frame;
		ADR_CDM_frame.mType = 2;//Index CDM
		ADR_CDM_frame.mStartingSampleInclusive = m_cdm_samplenummer_array[6];
		ADR_CDM_frame.mEndingSampleInclusive = m_cdm_samplenummer_array[12];
		ADR_CDM_frame.mData1 = m_cdm_daten;
		ADR_CDM_frame.mData2 = 7;//Index für die Anzahl der Bitstellen
		ADR_CDM_frame.mFlags = 3;//Index für die Textausgabe in BISSAnalyzerResults
		//mResults->AddChannelBubblesWillAppearOn (mSettings->mMaChannel);						
		//mResults->AddFrame( ADR_CDM_frame );
		//mResults->CommitResults();	
		AddMyFrame(ADR_CDM_frame,ulCDM);
		//-------------------------------------------------------------------------
		m_cdm.Reset( &m_cdm_daten, AnalyzerEnums::MsbFirst, 4);

		for (j = 13; j <= 16; j++)//CDM Adresse CRC
		{
			m_cdm.AddBit( m_cdm_array[j]);
		}

		Frame CRC_CDM_frame;
		CRC_CDM_frame.mType = 2;//Index CDM
		CRC_CDM_frame.mStartingSampleInclusive = m_cdm_samplenummer_array[13];
		CRC_CDM_frame.mEndingSampleInclusive = m_cdm_samplenummer_array[16];
		CRC_CDM_frame.mData1 = m_cdm_daten;
		CRC_CDM_frame.mData2 = 4;//Index für die Anzahl der Bitstellen
		CRC_CDM_frame.mFlags = 4;//Index für die Textausgabe in BISSAnalyzerResults
		//mResults->AddChannelBubblesWillAppearOn (mSettings->mMaChannel);						
		//mResults->AddFrame( CRC_CDM_frame );
		//mResults->CommitResults();	
		AddMyFrame(CRC_CDM_frame,ulCDM);
		//------------------------------------------------------------------------
		m_cdm.Reset( &m_cdm_daten, AnalyzerEnums::MsbFirst, 2);

		for (j = 17; j <= 18; j++)//Lesen, Schreiben CDM
		{
			m_cdm.AddBit( m_cdm_array[j]);
		}

		Frame RW_CDM_frame;
		RW_CDM_frame.mType = 2;//Index CDM
		RW_CDM_frame.mStartingSampleInclusive = m_cdm_samplenummer_array[17];
		RW_CDM_frame.mEndingSampleInclusive = m_cdm_samplenummer_array[18];
		RW_CDM_frame.mData1 = m_cdm_daten;
		RW_CDM_frame.mData2 = 2;//Index für die Anzahl der Bitstellen
		RW_CDM_frame.mFlags = 5;//Index für die Textausgabe in BISSAnalyzerResults
		//mResults->AddChannelBubblesWillAppearOn (mSettings->mMaChannel);						
		//mResults->AddFrame( RW_CDM_frame );
		//mResults->CommitResults();	
		AddMyFrame(RW_CDM_frame,ulCDM);
		//-----------------------------------------------------------------------
		int z = 0;

		if (m_cdm_array[18] == BIT_HIGH)//Wenn geschrieben wird RW = 01
		{

			while (m_cdm_array[j] == BIT_HIGH)//Start CDM auf 1 ? --> Abfrage Sequentieller Registerzugriff
		{
			////-------------------------------------------------------------------
			//m_cdm.Reset( &m_cdm_daten, AnalyzerEnums::MsbFirst, 8);

			//for (j = 20; j <= 27; j++)//zu schreibende Daten
			//{
			//	m_cdm.AddBit( m_cdm_array[j]);
			//}

			//Frame WDATA_CDM_frame;
			//WDATA_CDM_frame.mType = 2;//Index CDM
			//WDATA_CDM_frame.mStartingSampleInclusive = m_cdm_samplenummer_array[20];
			//WDATA_CDM_frame.mEndingSampleInclusive = m_cdm_samplenummer_array[27];
			//WDATA_CDM_frame.mData1 = m_cdm_daten;
			//WDATA_CDM_frame.mData2 = 8;//Index für die Anzahl der Bitstellen
			//WDATA_CDM_frame.mFlags = 6;//Index für die Textausgabe in BISSAnalyzerResults
			//mResults->AddChannelBubblesWillAppearOn (mSettings->mMaChannel);						
			//mResults->AddFrame( WDATA_CDM_frame );
			//mResults->CommitResults();	
			////------------------------------------------------------------------
			//m_cdm.Reset( &m_cdm_daten, AnalyzerEnums::MsbFirst, 4);

			//for (j = 28; j <= 31; j++)//CRC
			//{
			//	m_cdm.AddBit( m_cdm_array[j]);
			//}

			//Frame WCRC_CDM_frame;
			//WCRC_CDM_frame.mType = 2;//Index CDM
			//WCRC_CDM_frame.mStartingSampleInclusive = m_cdm_samplenummer_array[28];
			//WCRC_CDM_frame.mEndingSampleInclusive = m_cdm_samplenummer_array[31];
			//WCRC_CDM_frame.mData1 = m_cdm_daten;
			//WCRC_CDM_frame.mData2 = 4;//Index für die Anzahl der Bitstellen
			//WCRC_CDM_frame.mFlags = 7;//Index für die Textausgabe in BISSAnalyzerResults
			//mResults->AddChannelBubblesWillAppearOn (mSettings->mMaChannel);						
			//mResults->AddFrame( WCRC_CDM_frame );
			//mResults->CommitResults();		

			m_cdm.Reset( &m_cdm_daten, AnalyzerEnums::MsbFirst, 8);

			//while (m_cds_array[j] == BIT_LOW)//Beim Lesen Verarbeitungszeit überspringen
			//{
			//	j++;
			//}
			j++;//Start CDS überspringen
			for (z=j; z <= (j+8); z++)//Daten CDM
			{
				m_cdm.AddBit( m_cdm_array[z]);
			}

			Frame WDATA_CDM_frame;
			WDATA_CDM_frame.mType = 2;//Index CDM
			WDATA_CDM_frame.mStartingSampleInclusive = m_cdm_samplenummer_array[(z-9)];
			WDATA_CDM_frame.mEndingSampleInclusive = m_cdm_samplenummer_array[z-2];
			WDATA_CDM_frame.mData1 = m_cdm_daten;	
			WDATA_CDM_frame.mData2 = 8;//Index für die Anzahl der Bitstellen
			WDATA_CDM_frame.mFlags = 6;//Index für die Textausgabe in BISSAnalyzerResults
			//mResults->AddChannelBubblesWillAppearOn (mSettings->mMaChannel);	
			//mResults->AddFrame( WDATA_CDM_frame );
			//mResults->CommitResults();	
			AddMyFrame(WDATA_CDM_frame,ulCDM);
			//------------------------------------------------------------------------------
			m_cdm.Reset( &m_cdm_daten, AnalyzerEnums::MsbFirst, 4);

			for (j = (z-1); j <= (z+2); j++)//CRC Daten CDM
			{
				m_cdm.AddBit( m_cdm_array[j]);
			}


			Frame WCRC_CDM_frame;
			WCRC_CDM_frame.mType = 2;//Index CDM
			WCRC_CDM_frame.mStartingSampleInclusive = m_cdm_samplenummer_array[(z-1)];
			WCRC_CDM_frame.mEndingSampleInclusive = m_cdm_samplenummer_array[(z+2)];
			WCRC_CDM_frame.mData1 = m_cdm_daten;	
			WCRC_CDM_frame.mData2 = 4;//Index für die Anzahl der Bitstellen
			//WCRC_CDM_frame.mFlags = 4;//Index für die Textausgabe in BISSAnalyzerResults
			WCRC_CDM_frame.mFlags = 7;//Index für die Textausgabe in BISSAnalyzerResults
			//mResults->AddChannelBubblesWillAppearOn (mSettings->mMaChannel);	
			//mResults->AddFrame( WCRC_CDM_frame );		
			//mResults->CommitResults();	
			AddMyFrame(WCRC_CDM_frame,ulCDM);
			j++;//CDM Stop überspringen, somit "Zeiger" auf Start CDM bzw. CDS
		}
		}			
	}	
}
//---------------------------------------------------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------------------------------------------------
void BISSAnalyzer::BuiltCdsFrames()
//---------------------------------------------------------------------------------------------------------------------------------
{
	unsigned long ulCDS = 0;

	if ((m_cdm_array[1] == BIT_HIGH) && (m_cdm_array[2] == BIT_HIGH))//Datenanzeige nur bei Start,CTS = 1
	{
		U32 j = 0;

		m_cds.Reset( &m_cds_daten, AnalyzerEnums::MsbFirst, 8);

		for (j = 1; j <= 8; j++)//IDL
		{
			m_cds.AddBit( m_cds_array[j]);
		}

		Frame IDL_frame;
		IDL_frame.mType = 3;//Index CDS
		IDL_frame.mStartingSampleInclusive = m_cds_samplenummer_array[1];
		IDL_frame.mEndingSampleInclusive = m_cds_samplenummer_array[8];
		IDL_frame.mData1 = m_cds_daten;	
		IDL_frame.mData2 = 8;//Index für die Anzahl der Bitstellen
		IDL_frame.mFlags = 1;//Index für die Textausgabe in BISSAnalyzerResults
		//mResults->AddFrame( IDL_frame );
		//mResults->CommitResults();	
		AddMyFrame(IDL_frame,ulCDS);
		//--------------------------------------------------------------------------------
		m_cds.Reset( &m_cds_daten, AnalyzerEnums::MsbFirst, 8);

		for (j = 9; j <= 16; j++)//Low-Pegel nach IDL
		{
			m_cds.AddBit( m_cds_array[j]);
		}

		Frame NULL_frame;
		NULL_frame.mType = 3;//Index CDS
		NULL_frame.mStartingSampleInclusive = m_cds_samplenummer_array[9];
		NULL_frame.mEndingSampleInclusive = m_cds_samplenummer_array[16];
		NULL_frame.mData1 = m_cds_daten;	
		NULL_frame.mData2 = 8;//Index für die Anzahl der Bitstellen
		NULL_frame.mFlags = 2;//Index für die Textausgabe in BISSAnalyzerResults
		//mResults->AddFrame( NULL_frame );
		//mResults->CommitResults();	
		AddMyFrame(NULL_frame,ulCDS);
		//--------------------------------------------------------------------------------
		m_cds.Reset( &m_cds_daten, AnalyzerEnums::MsbFirst, 2);

		for (j = 17; j <= 18; j++)//Lesen, Schreiben CDS
		{
			m_cds.AddBit( m_cds_array[j]);
		}

		Frame RW_CDS_frame;
		RW_CDS_frame.mType = 3;//Index CDS
		RW_CDS_frame.mStartingSampleInclusive = m_cds_samplenummer_array[17];
		RW_CDS_frame.mEndingSampleInclusive = m_cds_samplenummer_array[18];
		RW_CDS_frame.mData1 = m_cds_daten;	
		RW_CDS_frame.mData2 = 2;//Index für die Anzahl der Bitstellen
		RW_CDS_frame.mFlags = 3;//Index für die Textausgabe in BISSAnalyzerResults
		//mResults->AddFrame( RW_CDS_frame );
		//mResults->CommitResults();	
		AddMyFrame(RW_CDS_frame,ulCDS);
		//-------------------------------------------------------------------------------
		
		//j=19;
		U32 z=0;

		while (m_cdm_array[j] == BIT_HIGH)//Start CDM auf 1 ? --> Abfrage Sequentieller Registerzugriff
		{
			m_cds.Reset( &m_cds_daten, AnalyzerEnums::MsbFirst, 8);

			while (m_cds_array[j] == BIT_LOW)//Beim Lesen Verarbeitungszeit überspringen
			{
				j++;
			}
			j++;//Start CDS überspringen
			for (z=j; z <= (j+8); z++)//Daten CDS
			{
				m_cds.AddBit( m_cds_array[z]);
			}

			Frame DATA_CDS_frame;
			DATA_CDS_frame.mType = 3;//Index CDS
			DATA_CDS_frame.mStartingSampleInclusive = m_cds_samplenummer_array[(z-9)];
			DATA_CDS_frame.mEndingSampleInclusive = m_cds_samplenummer_array[z-2];
			DATA_CDS_frame.mData1 = m_cds_daten;	
			DATA_CDS_frame.mData2 = 8;//Index für die Anzahl der Bitstellen
			DATA_CDS_frame.mFlags = 4;//Index für die Textausgabe in BISSAnalyzerResults
			//mResults->AddFrame( DATA_CDS_frame );
			//mResults->CommitResults();
			AddMyFrame(DATA_CDS_frame,ulCDS);
			//------------------------------------------------------------------------------
			m_cds.Reset( &m_cds_daten, AnalyzerEnums::MsbFirst, 4);

			for (j = (z-1); j <= (z+2); j++)//CRC CDS
			{
				m_cds.AddBit( m_cds_array[j]);
			}


			Frame CRC_CDS_frame;
			CRC_CDS_frame.mType = 3;//Index CDS
			CRC_CDS_frame.mStartingSampleInclusive = m_cds_samplenummer_array[(z-1)];
			CRC_CDS_frame.mEndingSampleInclusive = m_cds_samplenummer_array[(z+2)];
			CRC_CDS_frame.mData1 = m_cds_daten;	
			CRC_CDS_frame.mData2 = 4;//Index für die Anzahl der Bitstellen
			CRC_CDS_frame.mFlags = 5;//Index für die Textausgabe in BISSAnalyzerResults
			//mResults->AddFrame( CRC_CDS_frame );		
			//mResults->CommitResults();	
			AddMyFrame(CRC_CDS_frame,ulCDS);
			j++;//CDS Stop überspringen, somit "Zeiger" auf Start CDM bzw. CDS
		}
	}
	
}
//---------------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------------
void BISSAnalyzer::ShowCdmCds()
//---------------------------------------------------------------------------------------------------------------------------------
{
	unsigned long ulcdmframe_index = 0; 
	unsigned long ulcdsframe_index = 0; 

	while ( (ulcdmframe_index < mul_cdmframe_index) || (ulcdsframe_index < mul_cdsframe_index) )
	{
		//CDM
		if ( (m_faCDM[ulcdmframe_index].mEndingSampleInclusive < m_faCDS[ulcdsframe_index].mEndingSampleInclusive) && ( ulcdmframe_index < mul_cdmframe_index ) )
		{
			mResults->AddChannelBubblesWillAppearOn (mSettings->mMaChannel);						
			mResults->AddFrame( m_faCDM[ulcdmframe_index] );
			mResults->CommitResults();
			ulcdmframe_index ++;
		}	
		//CDS
		else
		{
			mResults->AddFrame( m_faCDS[ulcdsframe_index] );		
			mResults->CommitResults();
			ulcdsframe_index ++;
		}
	}
}
//---------------------------------------------------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------------------------------------------------
void BISSAnalyzer::AddMyFrame(Frame frame, int CDM_CDS)
//---------------------------------------------------------------------------------------------------------------------------------
{
	if (CDM_CDS == 1)//1=CDM
	{
		m_faCDM[mul_cdmframe_index] = frame;
		mul_cdmframe_index ++;
	}
	if (CDM_CDS == 0)//0=CDS
	{
		m_faCDS[mul_cdsframe_index] = frame;
		mul_cdsframe_index ++;
	}
}
//---------------------------------------------------------------------------------------------------------------------------------


bool BISSAnalyzer::NeedsRerun()
{
	return false;
}

U32 BISSAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 BISSAnalyzer::GetMinimumSampleRateHz()
{
	return 12000000;
}

const char* BISSAnalyzer::GetAnalyzerName() const
{
    return "BiSS C";
}

void BISSAnalyzer::SetupResults()
{
    mResults.reset( new BISSAnalyzerResults( this, mSettings.get() ) );
    SetAnalyzerResults( mResults.get() );

    mResults->AddChannelBubblesWillAppearOn( mSettings->mSloChannel );
}

const char* GetAnalyzerName()
{
	return "BiSS C";
}

Analyzer* CreateAnalyzer()
{
	return new BISSAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}


