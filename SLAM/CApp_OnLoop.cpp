#include "CApp.h"

#define DEMO 1

void CApp::OnLoop() {
	//SDL_Delay(20);
	uint32_t Time = SDL_GetTicks();

	float Zoom = 0.45F*float(Surf_Overlay->w)/LIDARCLIPPINGRANGE;

	PointClass* Points = NULL;
	IMUStateClass* IMUState = NULL;
	if ((Points = UTM30.GetPoints()) != NULL && (IMUState = sbgIMU.GetIMUState()) != NULL) {
		//Draw raw points
		int xMid = Surf_Overlay->w/2;
		int yMid = Surf_Overlay->h/2;
		for (uint16_t i = 0; i < LIDARPOINTCOUNT; i++) {
			int x = int(xMid + Zoom*Points[i].X);
			int y = int(yMid - Zoom*Points[i].Y); //Window axes are different
			CSurface::PutPixel(Surf_Overlay,x,y,255,255,255);
		}

		//-----------------------------------------------------------------------
		//------------------------SEGMENTATION IS CRUCIAL------------------------
		//Segmentation+Split & Merge
		SplitMergeItem* SplitMergeHead = NULL;
		SplitMergeItem* SplitMergeTail = NULL;
		uint16_t IndexStart = 0;
		for (uint16_t i = 0; i < LIDARPOINTCOUNT-1; i++) {
			if (Points[i].Range == 0) {
				//Current point is zero, skip
				IndexStart = i+1;
			} else {
				//if (Points[i+1].Range == 0 || i == LIDARPOINTCOUNT-2 || pow(Points[i].X-Points[i+1].X,2)+pow(Points[i].Y-Points[i+1].Y,2) > 40000) {
				//if (Points[i+1].Range == 0 || i == LIDARPOINTCOUNT-2 || abs(Points[i].Range-Points[i+1].Range) > 200) {
				if (Points[i+1].Range == 0 || i == LIDARPOINTCOUNT-2) {
					//Next point is zero OR last point of scan OR big distance between two consecutive points
					if (SplitMergeHead == NULL) {
						SplitMergeHead = SplitMerge(Points,IndexStart,i,100);
						SplitMergeHead->AOpen = true;
						SplitMergeTail = SplitMergeHead;
						while (SplitMergeTail->Next != NULL) {
							SplitMergeTail = SplitMergeTail->Next;
						}
						SplitMergeTail->BOpen = true;
					} else {
						SplitMergeTail->Next = SplitMerge(Points,IndexStart,i,100);
						SplitMergeTail->Next->AOpen = true;
						while (SplitMergeTail->Next != NULL) {
							SplitMergeTail = SplitMergeTail->Next;
						}
						SplitMergeTail->BOpen = true;
					}
					IndexStart = i+1;
				}
			}
		}
		//------------------------SEGMENTATION IS CRUCIAL------------------------
		//-----------------------------------------------------------------------

		//Feature extraction
		LinesHolderClass* LinesHolderLocal = LineFit(Points,SplitMergeHead);
		CornersHolderClass* CornersHolderLocal = CornerExtraction(LinesHolderLocal);

		char str[100];
		sprintf_s(str,"LineCount:%d",LinesHolderLocal->Count);
		CSurface::OnDraw(Surf_Display,TTF_RenderText_Blended(Font,str,FontColor),Surf_Display->w-120,Surf_Display->h-20);

		sprintf_s(str,"CornerCount:%d",CornersHolderLocal->Count);
		CSurface::OnDraw(Surf_Display,TTF_RenderText_Blended(Font,str,FontColor),Surf_Display->w-270,Surf_Display->h-20);

		
		//----------------------------------------------------------------------
		//------------------------Just for visualisation------------------------
		//Draw split and merge points
		SplitMergeItem* SplitMergeCurrent = SplitMergeHead;
		for (int i = 0; i < LinesHolderLocal->Count; i++) {
			//Draw split & merge points
			int x1 = int(xMid + Zoom*Points[SplitMergeCurrent->IndexA].X);
			int y1 = int(yMid - Zoom*Points[SplitMergeCurrent->IndexA].Y);
			int x2 = int(xMid + Zoom*Points[SplitMergeCurrent->IndexB].X);
			int y2 = int(yMid - Zoom*Points[SplitMergeCurrent->IndexB].Y);
			for (int k = -2; k <= 2; k++) {
				for (int j = -2; j <= 2; j++) {
					CSurface::PutPixel(Surf_Overlay,x1+j,y1+k,0,255,0);
					CSurface::PutPixel(Surf_Overlay,x2+j,y2+k,0,255,0);
				}
			}
			CSurface::DrawLine(Surf_Overlay,x1,y1,x2,y2,0,255,0);
			SplitMergeCurrent = SplitMergeCurrent->Next;
		}

		//Draw intersecting points and lines
		for (int i = 0; i < LinesHolderLocal->Count; i++) {
			if (LinesHolderLocal->Lines[i].Good) {
				int x1 = int(xMid + Zoom*LinesHolderLocal->Lines[i].PointA.X);
				int y1 = int(yMid - Zoom*LinesHolderLocal->Lines[i].PointA.Y);
				int x2 = int(xMid + Zoom*LinesHolderLocal->Lines[i].PointB.X);
				int y2 = int(yMid - Zoom*LinesHolderLocal->Lines[i].PointB.Y);
				for (int k = -2; k <= 2; k++) {
					for (int j = -2; j <= 2; j++) {
						CSurface::PutPixel(Surf_Overlay,x1+j,y1+k,255,0,0);
						CSurface::PutPixel(Surf_Overlay,x2+j,y2+k,255,0,0);
					}
				}
				CSurface::DrawLine(Surf_Overlay,x1,y1,x2,y2,255,0,0);
			}
		}
		
		//Draw corners
		for (int i = 0; i < CornersHolderLocal->Count; i++) {
			int x = int(xMid + Zoom*CornersHolderLocal->Corners[i].X);
			int y = int(yMid - Zoom*CornersHolderLocal->Corners[i].Y);
			for (int k = -3; k <= 3; k++) {
				for (int j = -3; j <= 3; j++) {
					CSurface::PutPixel(Surf_Overlay,x+j,y+k,0,0,255);
				}
			}
			//Draw heading
			float length = Zoom*500;
			int x2 = x+int(length*cos(CornersHolderLocal->Corners[i].Heading));
			int y2 = y-int(length*sin(CornersHolderLocal->Corners[i].Heading));
			CSurface::DrawLine(Surf_Overlay,x,y,x2,y2,0,0,255);
		}
		//------------------------Just for visualisation------------------------
		//----------------------------------------------------------------------

		
		//-----------------------------------------------------------------------------------------------------------------------
		//-----------------------------------------------------SCAN MATCHING-----------------------------------------------------
		HypothesisItem* HypothesisScanMatching = NULL;
		OdometryClass* Odometry = NULL;
		if (Previous.Corners != NULL && Previous.IMUState != NULL) {
			if (Previous.Corners->Count != 0) {
				if (CornersHolderLocal->Count != 0) {
					CornersHolderClass* CornersHolderIntermediate = new CornersHolderClass;
					CornersHolderIntermediate->Count = CornersHolderLocal->Count;
					CornersHolderIntermediate->Corners = new CornerClass[CornersHolderLocal->Count];
					float IMUTheta = Previous.IMUState->Yaw-IMUState->Yaw;
					if (IMUTheta > PI) {
						IMUTheta -= 2*PI;
					}
					if (IMUTheta < -PI) {
						IMUTheta += 2*PI;
					}
					float Sin_Theta = sin(IMUTheta);
					float Cos_Theta = cos(IMUTheta);
					for (uint16_t i = 0; i < CornersHolderIntermediate->Count; i++) {
						CornersHolderIntermediate->Corners[i].X = CornersHolderLocal->Corners[i].X*Cos_Theta-CornersHolderLocal->Corners[i].Y*Sin_Theta;
						CornersHolderIntermediate->Corners[i].Y = CornersHolderLocal->Corners[i].X*Sin_Theta+CornersHolderLocal->Corners[i].Y*Cos_Theta;
						CornersHolderIntermediate->Corners[i].Angle = CornersHolderLocal->Corners[i].Angle;
						CornersHolderIntermediate->Corners[i].Heading = CornersHolderLocal->Corners[i].Heading+IMUTheta;
						if (CornersHolderIntermediate->Corners[i].Heading > PI) {
							CornersHolderIntermediate->Corners[i].Heading -= 2*PI;
						}
						if (CornersHolderIntermediate->Corners[i].Heading < -PI) {
							CornersHolderIntermediate->Corners[i].Heading += 2*PI;
						}
						CornersHolderIntermediate->Corners[i].Covariance = CornersHolderLocal->Corners[i].Covariance;
					}
					HypothesisScanMatching = CornersAssociate(CornersHolderIntermediate,Previous.Corners,200); //Order: Detected, Known, Threshold

					//----------------------------------------------------------------------
					//------------------------Just for visualisation------------------------
					/*
					if (HypothesisScanMatching->AssociationCount != 0) {
						//Show associated corners
						for (int i = 0; i < CornersHolderLocal->Count; i++) {
							if (HypothesisScanMatching->Hypothesis[i] != 0) {
								int x = int(xMid + Zoom*CornersHolderLocal->Corners[i].X);
								int y = int(yMid - Zoom*CornersHolderLocal->Corners[i].Y);
								for (int k = -3; k <= 3; k++) {
									for (int j = -3; j <= 3; j++) {
										CSurface::PutPixel(Surf_Odometry,x+j,y+k,0,0,255);
									}
								}
								x = int(xMid + Zoom*Previous.Corners->Corners[HypothesisScanMatching->Hypothesis[i]-1].X);
								y = int(yMid - Zoom*Previous.Corners->Corners[HypothesisScanMatching->Hypothesis[i]-1].Y);
								for (int k = -3; k <= 3; k++) {
									for (int j = -3; j <= 3; j++) {
										CSurface::PutPixel(Surf_Odometry,x+j,y+k,0,255,255);
									}
								}
							}
						}
					}
					sprintf_s(str,"AssocCount:%d",HypothesisScanMatching->AssociationCount);
					CSurface::OnDraw(Surf_Display,TTF_RenderText_Blended(Font,str,FontColor),Surf_Display->w-410,Surf_Display->h-20);
					*/
					//------------------------Just for visualisation------------------------
					//----------------------------------------------------------------------

					if (HypothesisScanMatching->AssociationCount > 2) {
						//Odometry = ScanMatching(Points,Previous.Points,CornersHolderLocal,Previous.Corners,HypothesisScanMatching);
						Odometry = FindTranslation(Points,Previous.Points,CornersHolderIntermediate,Previous.Corners,HypothesisScanMatching);
						Odometry->Theta = IMUTheta;
					} else {
						//Insufficient associations for odometry
						if (Previous.Odometry != NULL) {
							Odometry = Previous.Odometry;
							Odometry->Theta = IMUTheta;
						}
					}
					delete CornersHolderIntermediate;
				} else {
					//No corners detected
					if (Previous.Odometry != NULL) {
						Odometry = Previous.Odometry;
						float IMUTheta = Previous.IMUState->Yaw-IMUState->Yaw;
						if (IMUTheta > PI) {
							IMUTheta -= 2*PI;
						}
						if (IMUTheta < -PI) {
							IMUTheta += 2*PI;
						}
						Odometry->Theta = IMUTheta;
					}
				}
			}
		}
		delete HypothesisScanMatching;
		//-----------------------------------------------------SCAN MATCHING-----------------------------------------------------
		//-----------------------------------------------------------------------------------------------------------------------

		
		//----------------------------------------------------------------------------------
		//-------------------------------Testing ScanMatching-------------------------------
#if DEMO == 0
		//Drawing on Surf_Odometry
		xMid = Surf_Odometry->w/2;
		yMid = Surf_Odometry->h/2;
		Zoom = 0.45F*float(Surf_Odometry->w)/LIDARCLIPPINGRANGE;

		if (Odometry != NULL) {
			float IMUTheta = Previous.IMUState->Yaw-IMUState->Yaw;
			if (IMUTheta > PI) {
				IMUTheta -= 2*PI;
			}
			if (IMUTheta < -PI) {
				IMUTheta += 2*PI;
			}
			sprintf_s(str,"Diff:%.1f",RAD2DEG(IMUTheta));
			CSurface::OnDraw(Surf_Display,TTF_RenderText_Blended(Font,str,FontColor),Surf_Display->w-410,Surf_Display->h-20);
		}
		//Draw current points
		for (uint16_t i = 0; i < LIDARPOINTCOUNT; i++) {
			int x = int(xMid + Zoom*Points[i].X);
			int y = int(yMid - Zoom*Points[i].Y); //Window axes are different
			CSurface::PutPixel(Surf_Odometry,x,y,255,255,255);
		}
		//Draw unaltered previous points (red)
		if (Previous.Points != NULL) {
			for (uint16_t i = 0; i < LIDARPOINTCOUNT; i++) {
				int x = int(xMid + Zoom*Previous.Points[i].X);
				int y = int(yMid - Zoom*Previous.Points[i].Y);
				CSurface::PutPixel(Surf_Odometry,x,y,255,0,0);
			}
		}
		//Draw points transformed with IMU yaw value (green)
		/*if (Previous.IMUState != NULL) {
			for (uint16_t i = 0; i < LIDARPOINTCOUNT; i++) {
				float drawX = Points[i].X;
				float drawY = Points[i].Y;
				drawX = drawX*cos(Previous.IMUState->Yaw-IMUState->Yaw)-drawY*sin(Previous.IMUState->Yaw-IMUState->Yaw);
				drawY = drawX*sin(Previous.IMUState->Yaw-IMUState->Yaw)+drawY*cos(Previous.IMUState->Yaw-IMUState->Yaw);
				int x = int(xMid + Zoom*drawX);
				int y = int(yMid - Zoom*drawY);
				CSurface::PutPixel(Surf_Odometry,x,y,0,255,0);
			}
		}*/
		//Draw transformed points (yellow)
		//Yellow should overlap with red
		if (Odometry != NULL) {
			for (uint16_t i = 0; i < LIDARPOINTCOUNT; i++) {
				float drawX = Points[i].X;
				float drawY = Points[i].Y;
				drawX = drawX*cos(Odometry->Theta)-drawY*sin(Odometry->Theta);
				drawY = drawX*sin(Odometry->Theta)+drawY*cos(Odometry->Theta);
				drawX += Odometry->X;
				drawY += Odometry->Y;
				int x = int(xMid + Zoom*drawX);
				int y = int(yMid - Zoom*drawY);
				CSurface::PutPixel(Surf_Odometry,x,y,255,255,0);
			}
			
		}
#endif
		//-------------------------------Testing ScanMatching-------------------------------
		//----------------------------------------------------------------------------------



		

#if DEMO == 1
		//-----------------------------------------------------------------------------
		//-------------------------------Particle Filter-------------------------------
		//Particle filter
		bool Resample = false;
		for (uint16_t i = 0; i < PARTICLECOUNT; i++) {
			Particles[i]->StateUpdate(Odometry);
			CornersHolderClass* CornersHolderGlobal = NULL;
			HypothesisItem* Hypothesis = NULL;
			if (CornersHolderLocal->Count != 0) {
				CornersHolderGlobal = Particles[i]->Loc2Glo(CornersHolderLocal);
				if (Particles[i]->CornersHolder->Count != 0) {
					Hypothesis = CornersAssociate(CornersHolderGlobal,Particles[i]->CornersHolder,400); //Watch the threshold
					if (Hypothesis->AssociationCount != 0) {
						for (uint16_t j = 0; j < CornersHolderGlobal->Count; j++) {
							if (Hypothesis->Hypothesis[j] != 0) {
								Particles[i]->Weight *= KalmanUpdate(&Particles[i]->CornersHolder->Corners[Hypothesis->Hypothesis[j]-1],&CornersHolderGlobal->Corners[j]);
							} else {
								//Not associated, new (or spurious)
								Particles[i]->AddCorner(&CornersHolderGlobal->Corners[j]);
								Particles[i]->Weight *= 1e-50; //Penalty
							}
							Resample = true;
						}
					} else {
						//All are "new"
						for (uint16_t j = 0; j < CornersHolderGlobal->Count; j++) {
							Particles[i]->AddCorner(&CornersHolderGlobal->Corners[j]);
						//	Particles[i]->Weight *= 1e-50; //Penalty
						}

						Particles[i]->Weight = 0; //Almost kills the particle


					}
				} else {
					//No known corners
					for (uint16_t j = 0; j < CornersHolderGlobal->Count; j++) {
						Particles[i]->AddCorner(&CornersHolderGlobal->Corners[j]);
					}
				}
			} else {
				//No corners detected
			}
			delete CornersHolderGlobal;
			delete Hypothesis;
		}
		//Resample
		if (Resample) {
			Particles = ParticleResample(Particles);
		}
		//-------------------------------Particle Filter-------------------------------
		//-----------------------------------------------------------------------------


		//----------------------------------------------------------------------------------
		//---------------------------------Visualising SLAM---------------------------------

		//Drawing on display
		xMid = Surf_Map->w/2;
		yMid = Surf_Map->h/2;
		Zoom = 0.4F*float(Surf_Map->w)/LIDARCLIPPINGRANGE;

		//Find particle with fewest corners
		uint16_t Chosen = 0;
		for (uint16_t i = 1; i < PARTICLECOUNT; i++) {
			if (Particles[i]->CornersHolder->Count < Particles[Chosen]->CornersHolder->Count) {
				Chosen = i;
			}
		}
		//Draw chosen particle's map
		for (uint16_t i = 0; i < Particles[Chosen]->CornersHolder->Count; i++) {
			int x = int(xMid + Zoom*Particles[Chosen]->CornersHolder->Corners[i].X);
			int y = int(yMid - Zoom*Particles[Chosen]->CornersHolder->Corners[i].Y);

			float length = Zoom*500;
			int x2 = x+int(length*cos(Particles[Chosen]->CornersHolder->Corners[i].Heading-Particles[Chosen]->CornersHolder->Corners[i].Angle/2+PI));
			int y2 = y-int(length*sin(Particles[Chosen]->CornersHolder->Corners[i].Heading-Particles[Chosen]->CornersHolder->Corners[i].Angle/2+PI));

			int x3 = x+int(length*cos(Particles[Chosen]->CornersHolder->Corners[i].Heading+Particles[Chosen]->CornersHolder->Corners[i].Angle/2+PI));
			int y3 = y-int(length*sin(Particles[Chosen]->CornersHolder->Corners[i].Heading+Particles[Chosen]->CornersHolder->Corners[i].Angle/2+PI));

			CSurface::DrawLine(Surf_Map,x,y,x2,y2,0,255,0);
			CSurface::DrawLine(Surf_Map,x,y,x3,y3,0,255,0);

			//sprintf_s(str,"%.1f",RAD2DEG(Particles[Chosen]->CornersHolder->Corners[i].Angle));
			//CSurface::OnDraw(Surf_Map,TTF_RenderText_Blended(Font,str,FontColor),x,y);
		}
		//Draw all particles relative to map
		for (uint16_t i = 0; i < PARTICLECOUNT; i++) {
			if (i != Chosen) {
				int x = int(xMid + Zoom*Particles[i]->X);
				int y = int(yMid - Zoom*Particles[i]->Y);
				for (int k = -1; k <= 1; k++) {
					for (int j = -1; j <= 1; j++) {
						CSurface::PutPixel(Surf_Map,x+j,y+k,255,255,255);
					}
				}
				//Draw heading
				float length = Zoom*500;
				int x2 = x+int(length*cos(Particles[i]->Theta+PI/2));
				int y2 = y-int(length*sin(Particles[i]->Theta+PI/2));
				CSurface::DrawLine(Surf_Map,x,y,x2,y2,255,255,255);
			}
		}
		int x = int(xMid + Zoom*Particles[Chosen]->X);
		int y = int(yMid - Zoom*Particles[Chosen]->Y);
		for (int k = -3; k <= 3; k++) {
			for (int j = -3; j <= 3; j++) {
				CSurface::PutPixel(Surf_Map,x+j,y+k,255,0,0);
			}
		}
		//Draw heading
		float length = Zoom*500;
		int x2 = x+int(length*cos(Particles[Chosen]->Theta+PI/2));
		int y2 = y-int(length*sin(Particles[Chosen]->Theta+PI/2));
		CSurface::DrawLine(Surf_Map,x,y,x2,y2,255,0,0);	
		
		//---------------------------------Visualising SLAM---------------------------------
		//----------------------------------------------------------------------------------
#endif
		









		//Record history
		if (Previous.Odometry != NULL && Previous.Odometry != Odometry) {
			delete Previous.Odometry;
		}
		Previous.Odometry = Odometry;

		if (Previous.Lines != NULL) {
			delete Previous.Lines;
		}
		Previous.Lines = LinesHolderLocal;
		if (Previous.Corners != NULL) {
			delete Previous.Corners;
		}
		Previous.Corners = CornersHolderLocal;
		if (Previous.IMUState != NULL) {
			delete Previous.IMUState;
		}
		Previous.IMUState = IMUState;
		if (Previous.Points != NULL) {
			delete Previous.Points;
		}
		Previous.Points = Points;

		//Deallocate
		LinkedListDeallocate(SplitMergeHead);
	}

	Time = SDL_GetTicks()-Time;
	char str[15];
	if (Time == 0) {
		sprintf_s(str,"FPS:1000");
	} else {
		sprintf_s(str,"FPS:%.1f",1000.0F/float(Time));
	}
	CSurface::OnDraw(Surf_Display,TTF_RenderText_Blended(Font,str,FontColor),0,Surf_Display->h-20);
}