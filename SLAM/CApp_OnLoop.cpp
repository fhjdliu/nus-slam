#include "CApp.h"

void CApp::OnLoop() {
	//SDL_Delay(20);
	uint32_t Time = SDL_GetTicks();

	float Zoom = 0.45F*float(Surf_Overlay->w)/LIDARCLIPPINGRANGE;

	PointClass* Points;
	if ((Points = UTM30.GetPoints()) != NULL) {
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
				//if (Points[i+1].Range == 0 || i == LIDARPOINTCOUNT-2/* || pow(Points[i].X-Points[i+1].X,2)+pow(Points[i].Y-Points[i+1].Y,2) > 40000*/) {
				if (Points[i+1].Range == 0 || i == LIDARPOINTCOUNT-2 || abs(Points[i].Range-Points[i+1].Range) > 200) {
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

		

		
		//Drawing directly on display now
		xMid = Surf_Display->w/2;
		yMid = Surf_Display->h/2;
		Zoom = 0.45F*float(Surf_Display->w)/LIDARCLIPPINGRANGE;

		//Draw points again
		for (uint16_t i = 0; i < LIDARPOINTCOUNT; i++) {
			int x = int(xMid + Zoom*Points[i].X);
			int y = int(yMid - Zoom*Points[i].Y); //Window axes are different
			CSurface::PutPixel(Surf_Display,x,y,255,255,255);
		}




		/*
		Odometry angle estimation using lines:
		Compare line thetas, if difference is greater than a small threshold, compare first and second.

		Transform corners for data association.
		*/

		float AngleGuess = 0;
		uint16_t AngleGuessCount = 0;
		if (Previous.Lines != NULL) {
			if (Previous.Lines->Count != 0) {
				if (LinesHolderLocal->Count != 0) {

					for (uint16_t i = 0; i < LinesHolderLocal->Count; i++) {
						for (int8_t j = -2; j < 2; j++) {
							if (i+j >= 0 && i+j < Previous.Lines->Count) {
								if (Previous.Lines->Lines[i+j].Good && LinesHolderLocal->Lines[i].Good) {
									if (abs(Previous.Lines->Lines[i+j].Length-LinesHolderLocal->Lines[i].Length) < 100) {
										if (Previous.Lines->Lines[i+j].Length > 1000 && LinesHolderLocal->Lines[i].Length > 1000) {
											float Difference = Previous.Lines->Lines[i+j].Theta-LinesHolderLocal->Lines[i].Theta;
											if (abs(Difference) < DEG2RAD(20)) {
												AngleGuess += Difference;
												AngleGuessCount++;
												break;
											}
										}
									}
								}
							}
						}
					}
				}
				AngleGuess /= AngleGuessCount;
			}
		}
		//Draw points transformed with AngleGuess (blue)
		for (uint16_t i = 0; i < LIDARPOINTCOUNT; i++) {

			float drawX = Points[i].X;
			float drawY = Points[i].Y;
			drawX = drawX*cos(AngleGuess)-drawY*sin(AngleGuess);
			drawY = drawX*sin(AngleGuess)+drawY*cos(AngleGuess);

			int x = int(xMid + Zoom*drawX);
			int y = int(yMid - Zoom*drawY);

			CSurface::PutPixel(Surf_Odometry,x,y,0,0,255);
		}

		if (Previous.Points != NULL) {
			//Draw unaltered previous points (red)
			for (uint16_t i = 0; i < LIDARPOINTCOUNT; i++) {
				int x = int(xMid + Zoom*Previous.Points[i].X);
				int y = int(yMid - Zoom*Previous.Points[i].Y);
				CSurface::PutPixel(Surf_Odometry,x,y,255,0,0);
			}
		}

		//Data association and scan matching
		HypothesisItem* HypothesisScanMatching = NULL;
		OdometryClass* Odometry = NULL;
		if (Previous.Corners != NULL) {
			if (Previous.Corners->Count != 0) {
				if (CornersHolderLocal->Count != 0) {
					HypothesisScanMatching = ScanMatchingCornersAssociate(CornersHolderLocal,Previous.Corners,300); //Order: Detected, Known, Threshold
					if (HypothesisScanMatching->AssociationCount != 0) {
						//Label associated corners
						for (int i = 0; i < CornersHolderLocal->Count; i++) {
							if (HypothesisScanMatching->Hypothesis[i] != 0) {
								int x = int(xMid + Zoom*CornersHolderLocal->Corners[i].X);
								int y = int(yMid - Zoom*CornersHolderLocal->Corners[i].Y);
								for (int k = -3; k <= 3; k++) {
									for (int j = -3; j <= 3; j++) {
										CSurface::PutPixel(Surf_Odometry,x+j,y+k,0,0,255);
									}
								}
								//Show neighbourhood points
								/*for (int j = -4; j <= 4; j++) {
									x = int(xMid + Zoom*Points[CornersHolderLocal->Corners[i].PointIndex+j].X);
									y = int(yMid - Zoom*Points[CornersHolderLocal->Corners[i].PointIndex+j].Y); //Window axes are different
									CSurface::PutPixel(Surf_Odometry,x,y,255,0,255);
								}*/
								x = int(xMid + Zoom*Previous.Corners->Corners[HypothesisScanMatching->Hypothesis[i]-1].X);
								y = int(yMid - Zoom*Previous.Corners->Corners[HypothesisScanMatching->Hypothesis[i]-1].Y);
								for (int k = -3; k <= 3; k++) {
									for (int j = -3; j <= 3; j++) {
										CSurface::PutPixel(Surf_Odometry,x+j,y+k,0,255,255);
									}
								}
								//Show neighbourhood points
								/*for (int j = -4; j <= 4; j++) {
									x = int(xMid + Zoom*Previous.Points[Previous.Corners->Corners[HypothesisScanMatching->Hypothesis[i]-1].PointIndex+j].X);
									y = int(yMid - Zoom*Previous.Points[Previous.Corners->Corners[HypothesisScanMatching->Hypothesis[i]-1].PointIndex+j].Y); //Window axes are different
									CSurface::PutPixel(Surf_Odometry,x,y,255,255,0);
								}*/
							}
						}
					}
					sprintf_s(str,"AssocCount:%d",HypothesisScanMatching->AssociationCount);
					CSurface::OnDraw(Surf_Display,TTF_RenderText_Blended(Font,str,FontColor),Surf_Display->w-410,Surf_Display->h-20);
					if (HypothesisScanMatching->AssociationCount > 2) {
						Odometry = ScanMatching(Points,Previous.Points,CornersHolderLocal,Previous.Corners,HypothesisScanMatching);
					} else {
						//Odometry = Previous.Odometry;
					}
				} else {
					//No corners detected
				}
			}
		}
		delete HypothesisScanMatching;
		








		/*---------------Works perfectly if simulated----------------
		PointClass TestPoints[LIDARPOINTCOUNT];
		float angle = DEG2RAD(1);
		for (uint16_t i = 0; i < LIDARPOINTCOUNT; i++) {
			TestPoints[i].X = Points[i].X*cos(angle)-Points[i].Y*sin(angle);
			TestPoints[i].Y = Points[i].X*sin(angle)+Points[i].Y*cos(angle);
		}
		OdometryClass* Odometry = ScanMatching(Points,TestPoints);
		float result = RAD2DEG(Odometry->Theta);

		//Draw test points (red)
		for (uint16_t i = 0; i < LIDARPOINTCOUNT; i++) {
			int x = int(xMid + Zoom*TestPoints[i].X);
			int y = int(yMid - Zoom*TestPoints[i].Y);
			CSurface::PutPixel(Surf_Display,x,y,255,0,0);
		}*/

		//-----------------Does not work in real life---------------
		//Yellow should overlap with red
		//OdometryClass* Odometry = NULL;
		/*if (Previous.Points != NULL) {
			/Odometry = ScanMatching(Points,Previous.Points); //Transformation from A to B*/
		if (Odometry != NULL) {
			float angle = RAD2DEG(Odometry->Theta);
			sprintf_s(str,"dTheta:%.3f",angle);
			CSurface::OnDraw(Surf_Display,TTF_RenderText_Blended(Font,str,FontColor),Surf_Display->w-550,Surf_Display->h-20);

			/*
			//Show centroids
			float XBarA = 0, YBarA = 0, XBarB = 0, YBarB = 0;
			for (int i = 0; i < LIDARPOINTCOUNT; i++) {
					XBarA += Points[i].X;
					YBarA += Points[i].Y;
					XBarB += Previous.Points[i].X;
					YBarB += Previous.Points[i].Y;
			}
			XBarA /= LIDARPOINTCOUNT;
			YBarA /= LIDARPOINTCOUNT;
			XBarB /= LIDARPOINTCOUNT;
			YBarB /= LIDARPOINTCOUNT;
			for (int k = -3; k <= 3; k++) {
				for (int j = -3; j <= 3; j++) {
					CSurface::PutPixel(Surf_Odometry,int(xMid+j+Zoom*XBarA),int(yMid+k-Zoom*YBarA),255,255,255);
					CSurface::PutPixel(Surf_Odometry,int(xMid+j+Zoom*XBarB),int(yMid+k-Zoom*YBarB),255,0,0);
				}
			}
			*/


			//Draw transformed points (yellow)
			for (uint16_t i = 0; i < LIDARPOINTCOUNT; i++) {

				float drawX = Points[i].X;
				float drawY = Points[i].Y;
				//drawX += 0.5F*Odometry->X;
				//drawY += 0.5F*Odometry->Y;
				drawX = drawX*cos(Odometry->Theta)-drawY*sin(Odometry->Theta);
				drawY = drawX*sin(Odometry->Theta)+drawY*cos(Odometry->Theta);
				//drawX += 0.5F*Odometry->X;
				//drawY += 0.5F*Odometry->Y;

				int x = int(xMid + Zoom*drawX);
				int y = int(yMid - Zoom*drawY);

				//int x = int(xMid + Zoom*(Points[i].X*cos(DEG2RAD(45))-Points[i].Y*sin(DEG2RAD(45))));
				//int y = int(yMid - Zoom*(Points[i].X*sin(DEG2RAD(45))+Points[i].Y*cos(DEG2RAD(45))));
				CSurface::PutPixel(Surf_Odometry,x,y,255,255,0);
			}
		}


		/*
		//Particle filter
		bool Resample = false;
		for (uint16_t i = 0; i < PARTICLECOUNT; i++) {
			Particles[i]->StateUpdate(Odometry);
			CornersHolderClass* CornersHolderGlobal = NULL;
			HypothesisItem* Hypothesis = NULL;
			if (CornersHolderLocal->Count != 0) {
				CornersHolderGlobal = Particles[i]->Loc2Glo(CornersHolderLocal);
				if (Particles[i]->CornersHolder->Count != 0) {
					Hypothesis = CornersAssociate(CornersHolderGlobal,Particles[i]->CornersHolder,100); //Watch the threshold
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
						//for (uint16_t j = 0; j < CornersHolderGlobal->Count; j++) {
						//	Particles[i]->AddCorner(&CornersHolderGlobal->Corners[j]);
						//	Particles[i]->Weight *= 1e-50; //Penalty
						//}

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













		//Drawing on display
		xMid = Surf_Display->w/2;
		yMid = Surf_Display->h/2;
		Zoom = 0.2F*float(Surf_Display->w)/LIDARCLIPPINGRANGE;

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

			CSurface::DrawLine(Surf_Display,x,y,x2,y2,0,255,0);
			CSurface::DrawLine(Surf_Display,x,y,x3,y3,0,255,0);

		}
		//Draw all particles relative to map
		for (uint16_t i = 0; i < PARTICLECOUNT; i++) {
			if (i != Chosen) {
				int x = int(xMid + Zoom*Particles[i]->X);
				int y = int(yMid - Zoom*Particles[i]->Y);
				for (int k = -1; k <= 1; k++) {
					for (int j = -1; j <= 1; j++) {
						CSurface::PutPixel(Surf_Display,x+j,y+k,255,255,255);
					}
				}
				//Draw heading
				float length = Zoom*500;
				int x2 = x+int(length*cos(Particles[i]->Theta+PI/2));
				int y2 = y-int(length*sin(Particles[i]->Theta+PI/2));
				CSurface::DrawLine(Surf_Display,x,y,x2,y2,255,255,255);
			}
		}
		int x = int(xMid + Zoom*Particles[Chosen]->X);
		int y = int(yMid - Zoom*Particles[Chosen]->Y);
		for (int k = -3; k <= 3; k++) {
			for (int j = -3; j <= 3; j++) {
				CSurface::PutPixel(Surf_Display,x+j,y+k,255,0,0);
			}
		}
		//Draw heading
		float length = Zoom*500;
		int x2 = x+int(length*cos(Particles[Chosen]->Theta+PI/2));
		int y2 = y-int(length*sin(Particles[Chosen]->Theta+PI/2));
		CSurface::DrawLine(Surf_Display,x,y,x2,y2,255,0,0);	
		*/
		










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