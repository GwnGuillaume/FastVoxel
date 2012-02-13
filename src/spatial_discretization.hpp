/*
 *     This file is part of FastVoxel.
 *
 *     FastVoxel is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     FastVoxel is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *     along with FastVoxel.  If not, see <http://www.gnu.org/licenses/>.
 * FastVoxel is a voxelisation library of polygonal 3d model and do volumes identifications.
 * It is dedicated to finite element solvers
 * @author Nicolas Fortin , Judica�l Picaut judicael.picaut (home) ifsttar.fr
 * Official repository is https://github.com/nicolas-f/FastVoxel
 */

#define USE_LOCAL_SMART_PTR
#ifndef USE_LOCAL_SMART_PTR
#include <boost/smart_ptr.hpp>
#define PTR boost::shared_ptr
#define PTR_ARR boost::shared_array
#else
#include "manager/smart_ptr.h"
#define PTR smart_ptr
#define PTR_ARR smart_ptr_ar
#endif

#include <Core/mathlib.h> //Mathlib de libinterface

#ifndef __SPATIAL_DISCRETIZATION__
#define __SPATIAL_DISCRETIZATION__

//#define __USE_MULTITHREAD__

namespace SpatialDiscretization
{
	typedef unsigned int cell_id_t;
	typedef short weight_t;
	const weight_t emptyValue=-1;
	struct domainInformation_t
	{
		domainInformation_t() :domainSize(0),weight(0){}
		domainInformation_t(const cell_id_t& matrixSize,const weight_t& defaultWeight=0) :domainSize(matrixSize),weight(defaultWeight){}
		cell_id_t domainSize;
		weight_t weight;
	};



	/**
	 * Cell correspond � un vecteur
	 * La particularit� de ce vecteur c'est de ne r�server que l'espace n�cessaire. C'est � dire seul les cellules comportant des informations sont instanci�s. Le gain d'espace m�moire n�cessaire est �lev�, au d�pend du temps d'acc�s aux cellules.
	 */

	template<class cellData_t>
	class Cell
	{
	   private :
	   cell_id_t Size;
	   PTR<Cell> nextCell;
	   cellData_t cellData;
	   public :
		   Cell() : cellData(emptyValue),Size(0) {}
	   Cell(const domainInformation_t& _DomainInformation ) : cellData(_DomainInformation.weight),Size(_DomainInformation.domainSize) {};
	   Cell(const domainInformation_t& _DomainInformation, const cell_id_t& cellSize, const cellData_t& _cellData  ) : cellData(_cellData),Size(cellSize) {};


	   void InsertCellAfter( const cell_id_t& nextCellSize, const cellData_t& nextCellData, const domainInformation_t& _DomainInformation)
	   {
			PTR<Cell> _nextCell(new Cell(_DomainInformation,nextCellSize,nextCellData));
			_nextCell->SetNextCell(this->nextCell);
			SetNextCell(_nextCell);
	   }
	   void DeleteNextCell()
	   {
		   if(nextCell.get()!=NULL)
			   this->nextCell=this->nextCell->nextCell;
	   }
	   void Resize( const cell_id_t& cellSize,const domainInformation_t& _DomainInformation)
	   {
		   this->Size=cellSize;
	   }

	   void SetNextCell( const PTR<Cell>& _nextCell)
	   {
			nextCell=_nextCell;
	   }

	   void Count( unsigned int& cnt)
	   {
			cnt++;
			if(nextCell.get())
			{
				(*nextCell).Count(cnt);
			}
	   }
	   /**
	    * Permet d'acceder au prochain noeud
		* @return Vrai si il y a un prochain noeud
		*/
	   bool Next(Cell** _nextCell)
	   {
			*_nextCell=nextCell.get();
			return IsNextCell();
	   }
	   /**
	    * Retourne la taille du noeud
		*/
		const cell_id_t& GetSize()
		{
			return Size;
		}
		/**
		 * Retourne la donn�e du noeud
		 */
		cellData_t& GetData()
		{
			return cellData;
		}
	   /**
		* Acceder � cellData
		*/
	   const cellData_t& operator [](const cell_id_t& id) const
	   {
		   cell_id_t curId=id;
		   const Cell* curCell=this;
		   do
		   {
			   if(curId<curCell->Size)
				   return curCell->cellData;
			   curId-=curCell->Size;
			   curCell=curCell->nextCell.get();
		   }while(curCell);
		#ifdef _DEBUG
		throw "Cell access out of array limit !";
		#endif
		return this->cellData;
	   }

	   bool IsNextCell()
	   {
			return this->nextCell.get()!=NULL;
	   }

		/**
		 * Modifie la valeur courante de la s�rie pour une nouvelle valeur
		 * @param newData Nouvelle valeur
		 */
		void SetData(const cellData_t& newData)
		{
			this->cellData=newData;
		}

        //TODO SetData(idStart,idEnd,newData)
		/**
		 * Set the cell value
		 */
		void SetData(const cell_id_t& id, const domainInformation_t& domainInformation, const cellData_t& newData)
		{
			cell_id_t curId=id;
			Cell* curCell=this;
			while(curCell)
			{
				if(curId>=curCell->Size)
				{
					//La cellule suivante peut etre fusionn� avec celle-ci dans les conditions suivante
					if(curId==curCell->Size && newData==curCell->cellData && curCell->nextCell.get()!=NULL && curCell->nextCell->cellData!=newData)
					{
						curCell->Size++;
						if(curCell->nextCell->Size==1)
						{
							curCell->nextCell=curCell->nextCell->nextCell;
							if(curCell->nextCell.get()!=NULL && curCell->nextCell->cellData==newData)
							{
								curCell->Size+=curCell->nextCell->Size;
								curCell->SetNextCell(curCell->nextCell->nextCell);
							}
						}else{
							curCell->nextCell->Size--;
						}
					}
					//Continuer vers la prochaine cellule
					if(curId<curCell->Size)
						return;
					curId=curId-curCell->Size;
					curCell=curCell->nextCell.get();
				}else{ //Cette cellule contient les anciennes donn�es
					if(newData==curCell->cellData)
						return;

					//Fusion de la cellule courante avec la prochaine cellule
					//Derni�re cellule, ou Size==1
					if(curId==curCell->Size-1)
					{
						if(curCell->nextCell.get()!=NULL && curCell->nextCell->cellData==newData)
						{
							if(curCell->Size>1)
							{
								//La cellule suivante absorbe notre derni�re position
								curCell->Size--;
								curCell->nextCell->Size++;
								return;
							}else{
								curCell->Size+=curCell->nextCell->Size;
								curCell->cellData=newData;
								curCell->DeleteNextCell();
							}
						}else{
							//On insere un �l�ment apr�s nous avec la nouvelle valeur
							if(curCell->Size>1)
							{
								curCell->InsertCellAfter(1,newData,domainInformation);
								curCell->Resize(curCell->Size-1,domainInformation);
								return;
							}else{
								curCell->cellData=newData;
								return;
							}
							return;
						}
					}else if(curId==0){
						//Premi�re position change d'�tat et taille courante > 1
						//On doit ins�rer un noeud
						curCell->InsertCellAfter(curCell->Size-1,curCell->cellData,domainInformation);
						curCell->cellData=newData;
						curCell->Resize(1,domainInformation);
						return;
					}else{
						//Cellule interm�diaire change d'�tat
						curCell->InsertCellAfter(curCell->Size-1-curId,curCell->cellData,domainInformation);
						curCell->InsertCellAfter(1,newData,domainInformation);
						curCell->Resize(curId,domainInformation);
						return;
					}
					return;
				}
			}
			throw "Cell access out of array limit !";
		}
	};

	/**
	 * CellArray est un vecteur simple
	 */
	template<class cellData_t>
	class CellArray
	{
	   private :
	   cell_id_t Size;
	   PTR_ARR<cellData_t> cellData;
	   public :
	   CellArray() : Size(0) {};
	   CellArray(const domainInformation_t& _DomainInformation ) : cellData(PTR_ARR<cellData_t>(new cellData_t[_DomainInformation.domainSize])),Size(_DomainInformation.domainSize)
	   {
			for(cell_id_t cellId=0;cellId<Size;cellId++)
				cellData[cellId].Resize(_DomainInformation.domainSize,_DomainInformation);
	   }
	   /**
		* Utilis� via la m�thode at, si une cellule doit etre cr��
		*/
	   void Resize( const cell_id_t& cellSize,const domainInformation_t& _DomainInformation) {
		   this->Size = cellSize;
		   cellData=PTR_ARR<cellData_t>(new cellData_t[this->Size]);
			for(cell_id_t cellId=0;cellId<Size;cellId++)
				cellData[cellId].Resize(this->Size,_DomainInformation);
	   }

		void Count( unsigned int& cnt)
		{
			for(cell_id_t cellId=0;cellId<Size;cellId++)
				cellData[cellId].Count(cnt);
		}
	   /**
		* Acceder � cellData
		*/
	   cellData_t& operator [](const cell_id_t& id)
	   {
			return cellData[id];
	   }
	   /**
		* Sp�cifier des donn�es, cr�� le noeud avec cet indice
		* Todo supprimer mod�le iteratif
		*/
	   cellData_t& at( const cell_id_t& id, const domainInformation_t& domainInformation)
	   {
	   		#ifdef _DEBUG
			if(id>=Size)
				throw "Cell access out of array limit !";
			#endif
			return cellData[id];
	   }
	};

	typedef PTR<Cell<weight_t> > zcell_ptr_t;
	typedef Cell<weight_t> zcell;
	typedef CellArray<CellArray<zcell> > weight_matrix;

}

#endif
