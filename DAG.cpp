/**
 * DAG.cpp
 * 
 * by Brent Williams
 */

#include "DAG.hpp"

DAG::DAG(const unsigned int levelsVal, const BoundingBox& boundingBoxVal, const std::vector<Triangle> triangles)
: boundingBox(boundingBoxVal),
   numLevels(levelsVal),
   size(pow(8, levelsVal)), 
   dimension(pow(2,levelsVal)),
   voxelWidth(0)
{
   if (numLevels <= 2)
   {
      std::string err("\nNumber of levels too small\n");
      std::cerr << err;
      throw std::out_of_range(err);
   }
   
   levels = new void*[numLevels-1]; // has the -1 because the last two levels are uint64's 
   sizeAtLevel = new unsigned int[numLevels-1](); // has the -1 because the last two levels are uint64's 
   boundingBox.square();
   voxelWidth = (boundingBox.maxs.x - boundingBox.mins.x) / dimension;
   build(triangles);
}

DAG::~DAG()
{
}

/**
 * Builds the DAG from a SVO
 * Tested: 
 */

void DAG::build(const std::vector<Triangle> triangles)
{
   SparseVoxelOctree svo(numLevels, boundingBox, triangles);
   root = svo.root;
   void** newLevels = new void*[numLevels-1]; // has the -1 because the last two levels are uint64's 
   unsigned int* newLevelSizes = new unsigned int[numLevels-1]();
   unsigned int newLevelIndex = numLevels-2;

   cerr << "Levels: " << endl;
   for (int i = 0; i < 2; ++i)
   {
      cerr << "\t" << i << ": " << svo.countAtLevel(i) << endl;
   }

   unsigned int numLeafs = size / 64;
   unsigned int sizeOfLeafs = numLeafs * sizeof(uint64_t);
   uint64_t* leafVoxels = (uint64_t*) svo.levels[numLevels-2]; // numLevels -2 b/c last 2 levels are together
   uint64_t* copyLeafVoxels = new uint64_t[numLeafs];
   unsigned int numChildren = numLeafs;
   unsigned int updateCount = 0;

   if (copyLeafVoxels == NULL)
   {
      cerr << "Unable to allocate copyLeafVoxels\n";
   }
   cerr << endl;

   int parentLevelNum = numLevels-3;
   SVONode* parentLevel = (SVONode*) svo.levels[parentLevelNum];
   unsigned int numParents = numLeafs / 8;

   std::cerr << "Starting leaf level (parentLevelNum: " << parentLevelNum << ")" << endl;
   std::cerr << "numParents: " << numParents << endl;
   std::cerr << "numChildren: " << numChildren << endl << endl;

   cerr << "\tSorting leaf nodes..." << endl;
   memcpy(copyLeafVoxels, leafVoxels, sizeOfLeafs);
   std::sort(copyLeafVoxels, copyLeafVoxels + numLeafs);
   cerr << "\tFinished sorting leaf nodes." << endl << endl;
   
   // cout << "\nLeafs:" << endl;
   // for (unsigned int i = 0; i < numLeafs; ++i)
   // {
   //    std::cout << copyLeafVoxels[i] << "\n";
   // }
   // std::cout << endl << endl;

   // Calculate the number of unique leafs
   cerr << "\tReducing leaf nodes..." << endl;
   unsigned int numUniqueLeafs = 1;
   for (unsigned int i = 0; i < numLeafs-1; ++i)
   {
      if (copyLeafVoxels[i] != copyLeafVoxels[i+1])
      {
         numUniqueLeafs++;
      }
   }
   std::cerr << "\t\tnumUniqueLeafs: " << numUniqueLeafs << endl;
   
   // Allocate the number of unique leafs and make copies of them
   uint64_t* uniqueLeafs = new uint64_t[numUniqueLeafs];
   unsigned int uniqueLeafIndex = 1;
   uniqueLeafs[0] = copyLeafVoxels[0];
   newLevels[newLevelIndex] = uniqueLeafs;
   newLevelSizes[newLevelIndex] = numUniqueLeafs;
   cout << "newLevels at leafs (newLevelIndex = " << newLevelIndex << ")" << endl;
   newLevelIndex--;

   for (unsigned int i = 0; i < numLeafs-1; ++i)
   {
      if (copyLeafVoxels[i] != copyLeafVoxels[i+1])
      {
         uniqueLeafs[uniqueLeafIndex] = copyLeafVoxels[i+1];
         uniqueLeafIndex++;
      }
   }

   //std::cerr << "uniqueLeafIndex: " << uniqueLeafIndex << endl;

   cout << "\nuniqueLeafs: " << endl;
   for (unsigned int i = 0; i < numUniqueLeafs; ++i)
   {
      std::cout << uniqueLeafs[i] << " => " << &(uniqueLeafs[i]) << "\n";
   }
   std::cout << endl << endl; 

   // Free the copy of the leafs
   delete [] copyLeafVoxels;
   cerr << "\tFinished reducing leaf nodes." << endl << endl;

   // Adjust the pointers of the level above in the svo
   cerr << "\tAdjusting parent node's pointer's to unique children..." << endl; 

   updateCount = 0;
   for (unsigned int i = 0; i < numParents; i++)
   {
      for (unsigned int j = 0; j < 8; j++)
      {
         if (parentLevel[i].childPointers[j] != NULL)
         {
            uint64_t childValue = *((uint64_t*)parentLevel[i].childPointers[j]);
            bool foundUpdate = false;
            // Search for the corresponding child value in the uniqueLeafs and replace the 
            // childpointer with a pointer to the unique leaf
            for (unsigned int k = 0; k < numUniqueLeafs && childValue != 0; k++)
            {
               if (uniqueLeafs[k] == childValue)
               {
                  parentLevel[i].childPointers[j] = &(uniqueLeafs[k]);
                  updateCount++;
                  cerr << "\t\tUpdate: " << updateCount << "\t=> "  << childValue << " &=> " << &(uniqueLeafs[k]) << endl;
                  foundUpdate = true;
               }
            }

            if (!foundUpdate)
            {
               cout << "!!!!!!!!! Did not find update for " << childValue << endl;
            }
         }
      }
   }
   cerr << "\tFinished adjusting parent node's pointer's to unique children." << endl << endl;
   std::cerr << "Finished leaf level (parentLevelNum: " << parentLevelNum << ")" << endl << endl << endl;

   parentLevelNum--;

////////////////////////////////////////////////////////////////////////////////////////////////
   //Adjust the pointers of the rest of the levels in the svo
   SVONode* childLevel;

   while (parentLevelNum >= 0)
   {
      childLevel = parentLevel;
      parentLevel = (SVONode*) svo.levels[parentLevelNum];
      numChildren = numParents;
      numParents /= 8;
      unsigned int sizeofChildren = sizeof(SVONode) * numChildren;

      std::cerr << "Starting parentLevelNum: " << parentLevelNum << endl;
      std::cerr << "numParents: " << numParents << endl;
      std::cerr << "numChildren: " << numChildren << endl << endl;

      // Sort the child level based on the pointers of each child node
      cerr << "\tSorting child nodes..." << endl;
      SVONode* copyChildNodes = new SVONode[numChildren];
      memcpy(copyChildNodes, childLevel, sizeofChildren);
      std::sort(copyChildNodes, copyChildNodes + numChildren);

      cout << "copyChildNodes (at parentLevel " << parentLevelNum << "): " << endl;
      for (unsigned int i = 0; i < numChildren; i++)
      {
         cout << &copyChildNodes[i] << ": ";
         copyChildNodes[i].printOneLine();
      }
      cout << endl;

      cerr << "\tFinished sorting child nodes." << endl << endl;

      //Reducing child nodes
      cerr << "\tReducing child nodes..." << endl;
      unsigned int numUniqueChildren = 1;
      for (unsigned int i = 0; i < numChildren-1; ++i)
      {
         if (copyChildNodes[i] != copyChildNodes[i+1])
         {
            numUniqueChildren++;
         }
      }
      cerr << "\t\tnumUniqueChildren: " << numUniqueChildren << endl;

      SVONode* uniqueChildren = new SVONode[numUniqueChildren];
      unsigned int uniqueChildIndex = 0;
      memcpy(&uniqueChildren[uniqueChildIndex], &copyChildNodes[0], sizeof(SVONode));
      uniqueChildIndex++;
      newLevels[newLevelIndex] = uniqueChildren;
      newLevelSizes[newLevelIndex] = numUniqueChildren;
      cout << "newLevels at newLevelIndex = " << newLevelIndex << endl;
      newLevelIndex--;
      for (unsigned int i = 0; i < numChildren-1; ++i)
      {
         if (copyChildNodes[i] != copyChildNodes[i+1])
         {
            memcpy(&uniqueChildren[uniqueChildIndex], &copyChildNodes[i+1], sizeof(SVONode));
            uniqueChildIndex++;
         }
      }
      delete [] copyChildNodes;

      cerr << "\tFinished reducing child nodes." << endl << endl;

      // Reducing child nodes
      cerr << "\tAdjusting parent node's pointer's to unique children..." << endl;
      updateCount = 0;
      bool foundUpdate = false;
      for (unsigned int i = 0; i < numParents; i++)
      {
         for (unsigned int j = 0; j < 8; j++)
         {
            if (parentLevel[i].childPointers[j] != NULL)
            {
               // Get a pointer to the address of the child value so you can replace it with the 
               // new unique child pointer
               SVONode childValue = *((SVONode*) parentLevel[i].childPointers[j]);

               // Search for the corresponding child value in the uniqueChildren and replace the 
               // childpointer with a pointer to the unique leaf
               for (unsigned int k = 0; k < numUniqueChildren; k++)
               {
                  if (uniqueChildren[k] == childValue)
                  {
                     parentLevel[i].childPointers[j] = &(uniqueChildren[k]);
                     updateCount++;
                     cerr << "\t\tUpdate: " << updateCount << " &=> " << &(uniqueChildren[k]) << endl;
                     foundUpdate = true;
                  }
               }
               if (!foundUpdate)
               {
                  cout << "!!!!!!!!! Did not find update for ";
                  childValue.printOneLine();
               }
            }
         }
      }

      cerr << "\tFinished adjusting parent node's pointer's to unique children." << endl << endl;

      std::cerr << "Finished parentLevelNum: " << parentLevelNum << endl << endl << endl;
      parentLevelNum--;
   }

   // Update the newLevels to include the root
   SVONode* newRoot = new SVONode;
   memcpy(newRoot, svo.levels[0], sizeof(SVONode));
   newLevels[0] = newRoot;
   newLevelSizes[0] = 1;

   cerr << "After updated SVO root:" << endl; 
   for (int i = 0; i < 8; ++i)
   {
      cerr << "\t" << newRoot->childPointers[i] << endl;
   }
   cerr << endl;

   cout << "Num unique nodes at level:" << endl;
   for (unsigned int i = 0; i < numLevels-1; ++i)
   {
      cout << "\t" << i << ": " << newLevelSizes[i] << endl;
   }

   // Go through each level and count the number of nonvoid pointers
   unsigned int levelIndex;
   for (levelIndex = 0; levelIndex < numLevels-2; levelIndex++)
   {
      unsigned int pointerCount = 0;
      for (unsigned int i = 0; i < newLevelSizes[levelIndex]; i++)
      {
         for (int j = 0; j < 8; j++)
         {
            if ( ((SVONode*) newLevels[levelIndex])[i].childPointers[j] != NULL)
            {
               pointerCount++;
            }
         }
      }

      // A level is made up of the pointers in the nodes and the masks of the nodes
      levels[levelIndex] = (void*)malloc((pointerCount * sizeof(void*)) + (newLevelSizes[levelIndex] * sizeof(uint64_t)));
      // cerr << "levelIndex = " << levelIndex << endl;
      // cerr << "pointerCount: " << pointerCount << endl;
      // cerr << "newLevelSizes[levelIndex]: " << newLevelSizes[levelIndex] << endl;
      cerr << levelIndex << ": " << ((pointerCount * sizeof(void*)) + (newLevelSizes[levelIndex] * sizeof(uint64_t))) / 8 << " uint64_t's or void*'s" << endl;
      sizeAtLevel[levelIndex] = ((pointerCount * sizeof(void*)) + (newLevelSizes[levelIndex] * sizeof(uint64_t))) / 8;
   }
   cerr << levelIndex << " (leafs): " << numUniqueLeafs << " uint64_t's or void*'s" << endl << endl;
   sizeAtLevel[levelIndex] = numUniqueLeafs;

   // Just use the already allocated leafs of the compacted SVO instead of allocating a new one
   levels[numLevels-2] = newLevels[numLevels-2];

   // Set the masks and pointers of the level above the leafs
   unsigned int currentLevelIndex = numLevels-3;
   cout << "Working at level above the leafs: " << currentLevelIndex << endl;
   uint64_t* maskPtr;
   uint64_t** currPtr = (uint64_t**) levels[currentLevelIndex];
   unordered_map<void*, void*>* leafParentMapping = new unordered_map<void*, void*>;
   for (unsigned int i = 0; i < newLevelSizes[currentLevelIndex]; i++)
   {
      uint64_t mask = 0;
      maskPtr = (uint64_t*) currPtr;
      leafParentMapping->insert( std::make_pair<void*,void*>( (void*) &((SVONode*) newLevels[currentLevelIndex])[i], (void*)maskPtr ) );
      cout << "Adding to Map: " << "( " <<  (void*) &((SVONode*) newLevels[currentLevelIndex])[i] << ", " << (void*)maskPtr << " )" << endl;
      currPtr++;
      for (int j = 0; j < 8; j++)
      {
         if ( ((SVONode*) newLevels[currentLevelIndex])[i].childPointers[j] != NULL)
         {
            // It is the same pointer as in the SVONodes because we are using the same leaf nodes
            *currPtr = (uint64_t*) ((SVONode*) newLevels[currentLevelIndex])[i].childPointers[j];
            mask |= 1;
            currPtr++;
         }
         mask <<= 1;
      }
      *maskPtr = mask;
   }

   unordered_map<void*, void*>* currLevelsMap = leafParentMapping;
   unordered_map<void*, void*>* prevLevelsMap = NULL;
   
   // For each levels nodes: set the mask 
   for (int levelIndex = numLevels-4; levelIndex >= 0; levelIndex--)
   {
      cout << "Working at level: " << levelIndex << endl;
      cerr << "Level " << levelIndex << endl;
      // Create a new map to 
      delete prevLevelsMap;
      prevLevelsMap = currLevelsMap;
      currLevelsMap = new unordered_map<void*, void*>;

      currPtr = (uint64_t**) levels[levelIndex];

      for (unsigned int i = 0; i < newLevelSizes[levelIndex]; i++)
      {
         uint64_t mask = 0;
         maskPtr = (uint64_t*) currPtr;
         currLevelsMap->insert( std::make_pair<void*,void*>( (void*) &((SVONode*) newLevels[levelIndex])[i], (void*)maskPtr ) );
         cout << "\tAdding to the map: ( " << &((SVONode*) newLevels[levelIndex])[i] << ", " << (void*)maskPtr << " )" << endl;
         currPtr++;
         
         cerr << "\t";
         for (int j = 0; j < 8; j++)
         {
            if ( ((SVONode*) newLevels[levelIndex])[i].childPointers[j] != NULL)
            {
               uint64_t toOr = 1 << j;
               cout << "Searching for: " << ((SVONode*) newLevels[levelIndex])[i].childPointers[j] << endl;
               *currPtr = (uint64_t*) prevLevelsMap->at( ((SVONode*) newLevels[levelIndex])[i].childPointers[j] );
               cout << "\tReturned value: " << *currPtr << endl;
               mask |= toOr;
               cerr << "1";
               currPtr++;
            }
            else 
            {
               cerr << "0";
            }
         }
         cerr << endl;
         cerr << "\tmask(" << maskPtr << "): " << mask << endl << endl;
         *maskPtr = mask;
      }
      cout << endl;
   }
   root=levels[0];
}


/**
 * Returns a boolean indicating whether the voxel at the given coordinate is 
 * set.
 *
 * Tested: 
 */
// bool DAG::isSet(unsigned int x, unsigned int y, unsigned int z)
// {
//    void* currentNode = root;
//    int currentLevel = numLevels;
//    unsigned int mortonIndex = mortonCode(x,y,z,currentLevel);
   
//    int divBy = pow(8, currentLevel-1);
//    int modBy = divBy;
//    int index = mortonIndex / divBy;
   
//    while (divBy >= 64)
//    {
//       if (!isChildSet((SVONode*)currentNode, index)) 
//       {
//          return false;
//       }
//       currentNode = (void*) ((SVONode*)currentNode)->childPointers[index];
//       modBy = divBy;
//       divBy /= 8;
//       index = (mortonIndex % modBy) / divBy;
//    }
//    index = mortonIndex % modBy;
//    return isLeafSet((uint64_t*)currentNode, index);
// }
bool DAG::isSet(unsigned int x, unsigned int y, unsigned int z)
{
   void* currentNode = root;
   int currentLevel = numLevels;
   unsigned int mortonIndex = mortonCode(x,y,z,currentLevel);
   
   int divBy = pow(8, currentLevel-1);
   int modBy = divBy;
   int index = mortonIndex / divBy;
   
   while (divBy >= 64)
   {
      if (!isChildSet(currentNode, index)) 
      {
         return false;
      }
      currentNode = (void*) getChildPointer(currentNode, index);
      modBy = divBy;
      divBy /= 8;
      index = (mortonIndex % modBy) / divBy;
   }
   index = mortonIndex % modBy;
   return isLeafSet((uint64_t*)currentNode, index);
}


void* DAG::getChildPointer(void* node, unsigned int index)
{
   void** pointer = (void**)node;
   pointer++;
   for (unsigned int i = 0; i < index; i++)
   {
      if (isChildSet(node, i))
      {
         pointer++;
      }
   }

   cout << "Getting childpointer " << index << " for node " << node << " = " << *pointer << endl;
   return *pointer;  
}


void DAG::printLevels() 
{
   // LEVEL 0
   void* node = levels[0];
   void** pointer = (void**)node;
   unsigned int childCount = 0;
   cout << "Level 0:" << endl;

   cout << node << " (";
   printMask(node);
   cout << "): ";

   for (unsigned int i = 0; i < 8; i++)
   {
      if (isChildSet(node, i))
      {
         pointer++;
         cout << *pointer << " ";
         childCount++;
      }
   }
   cout << endl << endl;

   // LEVEL 1
   node = levels[1];
   pointer = (void**)node;
   cout << "Level 1:" << endl;

   cout << node << " (";
   printMask(node);
   cout << "): ";
   for (unsigned int i = 0; i < 8; i++)
   {
      if (isChildSet(node, i))
      {
         pointer++;
         cout << *pointer << " ";
         childCount++;
      }
   }
   cout << endl << endl;

   // for (int i = 0; i < childCount; i++)
   // {
      
   // }

}

void DAG::printMask(void* node)
{
   uint64_t mask = *((uint64_t*)node);
   for (unsigned int i = 0; i < 8; i++)
   {
      if (isChildSet(node,i))
      {
         cout << "1";
      }
      else
      {
         cout << "0";
      }
   }

   cout << " " << mask;
}



/**
 * Returns a boolean indicating whether the node's leaf at the ith bit is set.
 *
 * Tested: 
 */
bool DAG::isLeafSet(uint64_t* node, unsigned int i)
{
   uint64_t leaf = (uint64_t) *node;
   uint64_t toAnd = (1L << i);
   return (leaf & toAnd) > 0; 
}


/**
 * Returns a boolean indicating whether the node's child is set.
 *
 * Tested: 
 */
bool DAG::isChildSet(void* node, unsigned int i)
{
   uint64_t toAnd = 1L << i;
   uint64_t mask = *((uint64_t*) node);
   //cout << "\t\t(" << mask <<  " & " << toAnd << ") = " << (mask | toAnd) << endl;
   return (mask & toAnd) != 0;
}



/**
 * Writes the voxel data to tga files where the file name is the z axis voxel number.
 *
 * Tested: 
 */
void DAG::writeImages()
{
   unsigned int i, j, k;
   
   for (k = 0; k < dimension; k++)
   {  
      cerr << "Witing image " << k << endl;
      Image image(dimension,dimension);
      for (j = 0; j < dimension; j++)
      {
         for (i = 0; i < dimension; i++)
         {
            if (isSet(i,j,k))
            {
               image.setColor(i,j, 1.0f,1.0f,1.0f);
            }
         }
      }
      char fileName[30];
      sprintf(fileName, "./images/%04d.tga",k);
      image.writeTGA((fileName));
   }
}


unsigned int DAG::getNumChildren(void* node)
{
   uint64_t mask = *((uint64_t*)node);
   unsigned int count = 0;
   for (int i = 0; i < 8; ++i)
   {
      uint64_t toOr = 1 << i;
      if (toOr | mask)
         count++;
   }
   return count;
}





























