//
// Created by Danielhu on 2018/1/20.
//
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <glm/glm.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/fast_square_root.hpp>
#include <Mesh.h>
#include "Octree.h"
#include "Utils.h"

using namespace glm;

const vec3 &min_offset_subdivision(int i) {
  static const vec3 offsets[8] = {
      vec3(0.f, 0.f, 0.f),
      vec3(0.f, 0.f, 1.f),
      vec3(0.f, 1.f, 0.f),
      vec3(0.f, 1.f, 1.f),
      vec3(1.f, 0.f, 0.f),
      vec3(1.f, 0.f, 1.f),
      vec3(1.f, 1.f, 0.f),
      vec3(1.f, 1.f, 1.f),
  };
  assert(i >= 0 && i < 8);
  return offsets[i];
};

const vec3 &directionMap(int i) {
  static const vec3 offsets[3] = {
      vec3(1.f, 0.f, 0.f),
      vec3(0.f, 1.f, 0.f),
      vec3(0.f, 0.f, 1.f),
  };
  assert(i >= 0 && i < 3);
  return offsets[i];
}

// from original dc implementation

const int edge_map[12][2] = {
    {0, 4}, {1, 5}, {2, 6}, {3, 7},    // x-axis
    {0, 2}, {1, 3}, {4, 6}, {5, 7},    // y-axis
    {0, 1}, {2, 3}, {4, 5}, {6, 7}     // z-axis
};

const int faceNodeOrder[3][4][4] = {
    {{0, 0, 1, 1}, {0, 0, 1, 1}, {0, 1, 1, 0}, {0, 1, 1, 0}},
    {{0, 1, 1, 0}, {0, 1, 1, 0}, {0, 0, 1, 1}, {0, 0, 1, 1}},
    {{0, 0, 1, 1}, {0, 0, 1, 1}, {0, 1, 1, 0}, {0, 1, 1, 0}},
};

const int cellProcFaceMask[12][3] =
    {{0, 4, 0}, {1, 5, 0}, {2, 6, 0}, {3, 7, 0}, {0, 2, 1}, {4, 6, 1}, {1, 3, 1}, {5, 7, 1}, {0, 1, 2}, {2, 3, 2},
     {4, 5, 2}, {6, 7, 2}};

const int cellProcEdgeMask[6][5] =
    {{0, 2, 3, 1, 0}, {4, 6, 7, 5, 0}, {0, 1, 5, 4, 1}, {2, 3, 7, 6, 1}, {0, 4, 6, 2, 2}, {1, 5, 7, 3, 2}};

const int faceProcFaceMask[3][4][3] = {
    {{4, 0, 0}, {5, 1, 0}, {6, 2, 0}, {7, 3, 0}},
    {{2, 0, 1}, {6, 4, 1}, {3, 1, 1}, {7, 5, 1}},
    {{1, 0, 2}, {3, 2, 2}, {5, 4, 2}, {7, 6, 2}}
};

const int edgeTestNodeOrder[4][2] = {{0, 1}, {3, 2}, {1, 2}, {0, 3}};

const vec3 &faceSubDivision(int dir, int i) {
  static const vec3 offsets[3][4] = {
      {vec3(0.f, 0.f, 0.f), vec3(0.f, 0.f, 1.f), vec3(0.f, 1.f, 0.f), vec3(0.f, 1.f, 1.f),},
      {vec3(0.f, 0.f, 0.f), vec3(1.f, 0.f, 0.f), vec3(0.f, 0.f, 1.f), vec3(1.f, 0.f, 1.f),},
      {vec3(0.f, 0.f, 0.f), vec3(0.f, 1.f, 0.f), vec3(1.f, 0.f, 0.f), vec3(1.f, 1.f, 0.f),},
  };
  assert(i >= 0 && i < 4);
  return offsets[dir][i];
};

const int faceProcEdgeMask[3][4][6] = {
    {{1, 4, 5, 1, 0, 1}, {1, 6, 7, 3, 2, 1}, {0, 4, 0, 2, 6, 2}, {0, 5, 1, 3, 7, 2}},
    {{0, 2, 0, 1, 3, 0}, {0, 6, 4, 5, 7, 0}, {1, 2, 6, 4, 0, 2}, {1, 3, 7, 5, 1, 2}},
    {{1, 1, 3, 2, 0, 0}, {1, 5, 7, 6, 4, 0}, {0, 1, 0, 4, 5, 1}, {0, 3, 2, 6, 7, 1}}
};

const vec3 &edgeProcDir(int i, int j) {
  const static vec3 dirs[3][4] = {
      {vec3(0.f, -1.f, -1.f), vec3(0.f, -1.f, 1.f), vec3(0.f, 1.f, 1.f), vec3(0.f, 1.f, -1.f),},
      {vec3(-1.f, 0.f, -1.f), vec3(-1.f, 0.f, 1.f), vec3(1.f, 0.f, 1.f), vec3(1.f, 0.f, -1.f),},
      {vec3(-1.f, -1.f, 0.f), vec3(1.f, -1.f, 0.f), vec3(1.f, 1.f, 0.f), vec3(-1.f, 1.f, 0.f),},
  };
  assert(i >= 0 && i < 3 && i >= 0 && i < 4);
  return dirs[i][j];
};

const int edgeProcEdgeMask[3][2][4] = {
    {{3, 1, 0, 2}, {7, 5, 4, 6}},
    {{5, 4, 0, 1}, {7, 6, 2, 3}},
    {{6, 2, 0, 4}, {7, 3, 1, 5}},
};

const int planeSpreadingDir[3][2][4] = {
    {{0, 2, 3, 1}, {4, 6, 7, 5}},
    {{0, 1, 5, 4}, {2, 3, 7, 6}},
    {{0, 4, 6, 2}, {1, 5, 7, 2}},
};
//
//const int planeSpreadingMask[8][8] = {
//    {0, 1, 2, 4, 3, 5, 6, 7},
//    {1, 0, 3, 5, 2, 4, 7, 6},
//    {2, 0, 3, 6, 1, 4, 7, 5},
//    {3, 1, 2, 7, 0, 5, 6, 4},
//    {4, 0, 5, 6, 1, 2, 7, 3},
//    {5, 1, 4, 7, 0, 3, 6, 2},
//    {6, 2, 4, 7, 0, 3, 5, 1},
//    {7, 3, 5, 6, 1, 2, 4, 0},
//};
//
//const int adjacentNodes[8][8] = {
//    {0, 1, 1, 0, 1, 0, 0, 0,},
//    {1, 0, 0, 1, 0, 1, 0, 0,},
//    {1, 0, 0, 1, 0, 0, 1, 0,},
//    {0, 1, 1, 0, 0, 0, 0, 1,},
//    {1, 0, 0, 0, 0, 1, 1, 0,},
//    {0, 1, 0, 0, 1, 0, 0, 1,},
//    {0, 0, 1, 0, 1, 0, 0, 1,},
//    {0, 0, 0, 1, 0, 1, 1, 0,},
//};

const int dirRelatedEdge[8][8][3] = {
    {
        {-1, -1, -1}, {-1, 2, 6}, {-1, 1, 10}, {-1, -1, 0},
        {-1, 5, 9}, {-1, -1, 4}, {-1, -1, 8}, {0, 4, 8},
    },
    {
        {-1, 3, 11}, {-1, -1, -1}, {-1, -1, 1}, {-1, 0, 10},
        {-1, -1, 5}, {-1, 4, 9}, {1, 5, 8}, {-1, -1, 8},
    },
    {
        {-1, 11, 3}, {-1, -1, 2}, {-1, -1, -1}, {-1, 0, 6},
        {-1, -1, 9}, {2, 4, 9}, {-1, 5, 8}, {-1, -1, 4},
    },
    {
        {-1, -1, 3}, {-1, 2, 11}, {-1, 1, 7}, {-1, -1, -1},
        {3, 5, 9}, {-1, -1, 9}, {-1, -1, 5}, {-1, 4, 8},
    },
    {
        {-1, 7, 11}, {-1, -1, 5}, {-1, -1, 10}, {0, 6, 10},
        {-1, -1, -1}, {-1, 2, 4}, {-1, 1, 8}, {-1, -1, 0},
    },
    {
        {-1, -1, 7}, {-1, 0, 11}, {1, 7, 10}, {-1, -1, 10},
        {-1, 3, 5}, {-1, -1, -1}, {-1, -1, 1}, {-1, 1, 8}
    },
    {
        {-1, -1, 11}, {2, 6, 11}, {-1, 7, 10}, {-1, -1, 6},
        {-1, 3, 9}, {-1, -1, 2}, {-1, -1, -1}, {-1, 1, 4},
    },
    {
        {3, 7, 11}, {-1, -1, 11}, {-1, -1, 7}, {-1, 6, 10},
        {-1, -1, 3}, {-1, 2, 9}, {-1, 1, 5}, {-1, -1, -1},
    }
};

const int processEdgeMask[3][4] = {{3, 2, 1, 0}, {7, 5, 6, 4}, {11, 10, 9, 8}};

//const int triangleIndices[6] = {0, 1, 2, 0, 2, 3};
//const int triangleIndicesFlip[6] = {0, 3, 2, 0, 2, 1};

glm::vec3 min_select(glm::vec3 a, glm::vec3 b, bool &flag) {
  if (a.x >= b.x && a.y >= b.y && a.z >= b.z) {
    return b;
  }
  if (a.x <= b.x && a.y <= b.y && a.z <= b.z) {
    return a;
  }
  flag = false;
  return vec3();
}

glm::vec3 max_select(glm::vec3 a, glm::vec3 b, bool &flag) {
  if (a.x >= b.x && a.y >= b.y && a.z >= b.z) {
    return a;
  }
  if (a.x <= b.x && a.y <= b.y && a.z <= b.z) {
    return b;
  }
  flag = false;
  return vec3();
}

bool Octree::getSelfQef(Octree *node, Topology *g, QefSolver &qef) {
  vec3 corners[8];
  int8_t mtlID = g->getMaterialID();
  for (int i = 0; i < 8; ++i) {
    corners[i] = min_offset_subdivision(i) * node->size + node->min;
    node->cornerSigns[i] = (uint8_t) (g->value(corners[i]) > 0. ? mtlID : 0);
  }
  bool intersect = false;
  for (int i = 0; i < 7; ++i) {
    if (node->cornerSigns[i] != node->cornerSigns[i + 1]) {
      intersect = true;
    }
  }
  if (!intersect) {
    return false;
  }
  for (int i = 0; i < 12; ++i) {
    vec3 p1 = corners[edge_map[i][0]];
    vec3 p2 = corners[edge_map[i][1]];
    if (node->cornerSigns[edge_map[i][0]] != node->cornerSigns[edge_map[i][1]]) {
      vec3 p, n;
      if (g->solve(p1, p2, p)) {
        g->normal(p, n);
        qef.add(p, n);
      }
    }
  }
  return true;
}

Octree *Octree::buildWithTopology(
    glm::vec3 min,
    vec3 size,
    int depth,
    Topology *topology,
    int &losslessCut) {
  auto root = buildRecursively(min, size, depth, topology);
  root = losslessCompress(root, -1, topology, losslessCut);
  return root;
}

Octree *Octree::buildRecursively(glm::vec3 min, vec3 size, int depth, Topology *topology) {
  auto root = new Octree(min, size, depth);
  assert(depth > 0);
  if (depth == 1) {
    if (!getSelfQef(root, topology, root->qef)) {
      root->internal = true;
      return root;
    }
    calHermite(root, &root->qef, topology, &root->vertex);
    root->clusterQef->set(root->qef);
    root->isLeaf = true;
    return root;
  }
  for (int i = 0; i < 8; ++i) {
    root->children[i] =
        buildRecursively(min + min_offset_subdivision(i) * size / 2.f, size / 2.f, depth - 1, topology);
    root->cornerSigns[i] = root->children[i]->cornerSigns[i];
    root->children[i]->childIndex = static_cast<int8_t>(i);
  }
  root->isLeaf = false;
  return root;
}

Octree *Octree::losslessCompress(Octree *root,
                                 float threshold,
                                 Topology *topology,
                                 int &count) {
  if (!root) {
    return nullptr;
  }
  if (root->isLeaf) {
    if (root->internal) {
      count++;
      return nullptr;
    }
    return root;
  }

  bool internal = true;

  for (int i = 0; i < 8; ++i) {

    auto result = losslessCompress(root->children[i], threshold, topology, count);
    if (result == nullptr) {
      root->children[i] = nullptr;
    }
    internal = internal && (root->children[i] == nullptr);
  }

  if (internal) {
    count++;
    return nullptr;
  }

  return root;
}

int Octree::simplify(Octree *root, float threshold, Topology *geometry) {
  if (!root) {
    return 0;
  }
  if (root->isLeaf) {
    return 0;
  }
  QefSolver sum;
  int reduction = 0;
  int childCount = 0;
  for (auto &child : root->children) {
    reduction += simplify(child, threshold, geometry);
    if (child) {
      childCount++;
      sum.combine(child->qef);
    }
  }
  vec3 tempP;
  sum.solve(tempP, root->error);
  root->qef.set(sum);
  float value = geometry->value(tempP);
  value = 0.f;
  if (root->error + abs(value) < threshold) {
    // getSelfQef(root, geometry, root->qef);
    calHermite(root, &sum, geometry, &root->vertex);
    reduction += childCount - 1;
    for (auto &child: root->children) {
      child = nullptr;
    }
    root->isLeaf = true;
    root->clusterQef->set(root->qef);
  }
  return reduction;
}

void Octree::reverseExtendedSimplify(Octree *root, Topology *g) {
  if (!root) {
    return;
  }
  if (root->isLeaf) {
    if (root->cluster->size() > 1) {
      root->cluster = new std::unordered_set<Octree *>({root});
      root->clusterVertex = &root->vertex;
      calHermite(root, &root->qef, g, &root->vertex);
      root->clusterQef = new QefSolver();
      root->clusterQef->set(root->qef);
      root->clusterMin = new glm::vec3(root->min);
      root->clusterSize = new glm::vec3(root->size);
    }
    return;
  }
  for (int i = 0; i < 8; ++i) {
    reverseExtendedSimplify(root->children[i], g);
  }
}

Octree *Octree::extendedSimplify(Octree *root,
                                 float threshold,
                                 Topology *geometry,
                                 int &count) {
  edgeClassifier(root, threshold, geometry);
  std::unordered_set<std::unordered_set<Octree *> *> clusters;
  edgeCluster(root, geometry, count, clusters);
  return root;
}

Octree *Octree::edgeClassifier(Octree *root,
                               float threshold,
                               Topology *geometry) {
  if (!root || root->isLeaf) {
    return root;
  }
  for (int i = 0; i < 8; ++i) {
    root->children[i] = edgeClassifier(root->children[i], threshold, geometry);
  }
  for (int i = 0; i < 12; ++i) {
    auto &a = root->children[cellProcFaceMask[i][0]];
    auto &b = root->children[cellProcFaceMask[i][1]];
    auto faceMin = root->min
        + directionMap(cellProcFaceMask[i][2]) * root->size / 2.f
        + faceSubDivision(cellProcFaceMask[i][2], i % 4) * root->size / 2.f;
    const int dir = cellProcFaceMask[i][2];
    edgeCollapse(a,
                 b,
                 dir,
                 threshold,
                 geometry,
                 faceMin,
                 root->size.x / 2.f);
  }
  return root;
}

void Octree::edgeCluster(Octree *root,
                         Topology *geometry,
                         int &count,
                         std::unordered_set<std::unordered_set<Octree *> *> &clusters) {
  if (!root) {
    return;
  }
  if (root->isLeaf) {
    if (clusters.find(root->cluster) != clusters.end()) {
      return;
    }
    clusters.insert(root->cluster);
    count += std::max(0, (int) root->cluster->size() - 1);
    if (root->cluster->size() > 1) {
      calHermite(root, root->clusterQef, geometry, root->clusterVertex);
      assert(root->clusterQef->pointCount != 0);
    }
  }
  for (int i = 0; i < 8; ++i) {
    edgeCluster(root->children[i], geometry, count, clusters);
  }
}

void Octree::cubeExtensionTest(Octree *a, Octree *b, int dir, float minSize) {
//  vec3 combineMin = glm::min(*a->clusterMin, *b->clusterMin);
//  vec3 combineSize = glm::max(*a->clusterMin + *a->clusterSize, *b->clusterMin + *b->clusterSize) - combineMin;
//  for (int i = 0; i < 8; ++i) {
//    vec3 corner = min_offset_subdivision(i) * combineSize + combineMin;
//
//  }

}

void Octree::edgeCollapse(Octree *&a,
                          Octree *&b,
                          int dir,
                          float threshold,
                          Topology *geometry,
                          vec3 faceMin,
                          float faceSize) {
  if (!a || !b) {
    return;
  }
  if (!a->isLeaf || !b->isLeaf) {
    for (int i = 0; i < 4; ++i) {
      vec3 subMin = faceMin + 0.5f * faceSize * faceSubDivision(dir, i);
      float subSize = faceSize / 2.f;

      if (!a->isLeaf && !b->isLeaf) {
        edgeCollapse(a->children[faceProcFaceMask[dir][i][0]],
                     b->children[faceProcFaceMask[dir][i][1]],
                     dir,
                     threshold,
                     geometry,
                     subMin,
                     subSize
        );
      } else if (a->isLeaf) {
        edgeCollapse(a, b->children[faceProcFaceMask[dir][i][1]], dir, threshold, geometry, subMin, subSize);
      } else {
        edgeCollapse(a->children[faceProcFaceMask[dir][i][0]], b, dir, threshold, geometry, subMin, subSize);
      }
    }
    return;
  }

  auto lastfaceNodeEndSign = -1;
  bool connectivity = false;

  for (int i = 0; i < 4; ++i) {
    auto faceNodeEndSign = 0;
    vec3 samplePoint = faceSubDivision(dir, i) * faceSize + faceMin;
    float density = geometry->value(samplePoint);
    if (density > 0) {
      faceNodeEndSign = geometry->getMaterialID();
    } else {
      faceNodeEndSign = 0;
    }
    if (faceNodeEndSign != lastfaceNodeEndSign && lastfaceNodeEndSign != -1) {
      connectivity = true;
    }
    lastfaceNodeEndSign = faceNodeEndSign;
  }

  if (!connectivity) {
    return;
  }

  if (a->cluster == b->cluster) {
    return;
  }

  vec3 minOffset = *b->clusterMin - *a->clusterMin;
  vec3 maxOffset = *b->clusterMin + *b->clusterSize - (*a->clusterMin + *a->clusterSize);

  for (int i = 0; i < 3; ++i) {
    if (i != dir) {
      if (minOffset[i] != 0 || maxOffset[i] != 0) {
        return;
      }
    }
  }

  QefSolver edgeQEF;
  edgeQEF.combine(*a->clusterQef);
  edgeQEF.combine(*b->clusterQef);
  vec3 combineP;
  float combineError;
  edgeQEF.solve(combineP, combineError);
  if (combineError < threshold) {
    combine(a, b, geometry);
  }
}

Mesh *Octree::generateMesh(Octree *root,
                           Topology *geometry,
                           int &intersectionPreservingVerticesCount,
                           bool intersectionFree) {
  assert(root);
  auto *mesh = new Mesh();
  std::unordered_set<Vertex *> indexed;
  EdgePolygonSet edgePolygonSet;
  generateVertexIndices(root, mesh, geometry, indexed);
  contourCell(root, mesh, geometry, intersectionPreservingVerticesCount, edgePolygonSet, intersectionFree);
  return mesh;
}

void Octree::generateVertexIndices(Octree *node,
                                   Mesh *mesh,
                                   Topology *geometry,
                                   std::unordered_set<Vertex *> &indexed) {
  if (!node) {
    return;
  }
  for (int i = 0; i < 8; ++i) {
    generateVertexIndices(node->children[i], mesh, geometry, indexed);
  }
  if (node->isLeaf) {
    if (indexed.find(node->clusterVertex) != indexed.end()) {
      return;
    }
    indexed.insert(node->clusterVertex);
    node->clusterVertex->vertexIndex = static_cast<unsigned int>(mesh->positions.size());
    mesh->positions.push_back(node->clusterVertex->hermiteP);
    mesh->normals.push_back(node->clusterVertex->hermiteN);
  }
}

void Octree::contourCell(Octree *root,
                         Mesh *mesh,
                         Topology *geometry,
                         int &intersectionPreservingVerticesCount,
                         EdgePolygonSet &edgePolygonSet,
                         bool intersectionFree) {
  if (!root || root->isLeaf) {
    return;
  }
  for (int i = 0; i < 8; ++i) {
    contourCell(root->children[i], mesh, geometry, intersectionPreservingVerticesCount, edgePolygonSet, intersectionFree);
  }
  for (int i = 0; i < 12; ++i) {
    Octree *nodes[2] = {
        root->children[cellProcFaceMask[i][0]],
        root->children[cellProcFaceMask[i][1]],
    };
    contourFace(nodes,
                cellProcFaceMask[i][2],
                mesh,
                geometry,
                intersectionPreservingVerticesCount,
                edgePolygonSet,
                intersectionFree);
  }
  for (int i = 0; i < 6; ++i) {
    Octree *nodes[4];
    for (int j = 0; j < 4; ++j) {
      nodes[j] = root->children[cellProcEdgeMask[i][j]];
    }
    contourEdge(nodes,
                cellProcEdgeMask[i][4],
                mesh,
                geometry,
                intersectionPreservingVerticesCount,
                edgePolygonSet,
                intersectionFree);
  }
}

void Octree::contourFace(Octree *nodes[2],
                         int dir,
                         Mesh *mesh,
                         Topology *geometry,
                         int &intersectionPreservingVerticesCount,
                         EdgePolygonSet &edgePolygonSet,
                         bool intersectionFree) {
  if (!nodes[0] || !nodes[1]) {
    return;
  }
  if (nodes[0]->isLeaf && nodes[1]->isLeaf) {
    return;
  }
  // the subdivision of a face resulting 4 child faces;
  for (int i = 0; i < 4; ++i) {
    Octree *subdivision_face[2] = {nodes[0], nodes[1]};
    for (int j = 0; j < 2; ++j) {
      if (!subdivision_face[j]->isLeaf) {
        subdivision_face[j] = subdivision_face[j]->children[faceProcFaceMask[dir][i][j]];
      }
    }
    contourFace(subdivision_face,
                faceProcFaceMask[dir][i][2],
                mesh,
                geometry,
                intersectionPreservingVerticesCount,
                edgePolygonSet,
                intersectionFree);
  }
  for (int i = 0; i < 4; ++i) {
    Octree *edge_nodes[4];
    const int c[4] =
        {
            faceProcEdgeMask[dir][i][1],
            faceProcEdgeMask[dir][i][2],
            faceProcEdgeMask[dir][i][3],
            faceProcEdgeMask[dir][i][4],
        };
    for (int j = 0; j < 4; ++j) {
      const int order = faceNodeOrder[dir][i][j];
      if (nodes[order]->isLeaf) {
        edge_nodes[j] = nodes[order];
      } else {
        edge_nodes[j] = nodes[order]->children[c[j]];
      }
    }
    contourEdge(edge_nodes,
                faceProcEdgeMask[dir][i][5],
                mesh,
                geometry,
                intersectionPreservingVerticesCount,
                edgePolygonSet,
                intersectionFree);
  }
}

void Octree::contourEdge(Octree *nodes[4],
                         int dir,
                         Mesh *mesh,
                         Topology *geometry,
                         int &intersectionPreservingVerticesCount,
                         EdgePolygonSet &edgePolygonSet,
                         bool intersectionFree) {
  if (!nodes[0] || !nodes[1] || !nodes[2] || !nodes[3]) {
    return;
  }

  if (nodes[0]->isLeaf && nodes[1]->isLeaf && nodes[2]->isLeaf && nodes[3]->isLeaf) {
    generateQuad(nodes, dir, mesh, geometry, intersectionPreservingVerticesCount, edgePolygonSet, intersectionFree);
    return;
  }

  // the subdivision of a edge resulting 2 child edges;
  for (int i = 0; i < 2; ++i) {
    Octree *subdivision_edge[4];
    for (int j = 0; j < 4; ++j) {
      if (!nodes[j]->isLeaf) {
        subdivision_edge[j] = nodes[j]->children[edgeProcEdgeMask[dir][i][j]];
      } else {
        subdivision_edge[j] = nodes[j];
      }
    }
    contourEdge(subdivision_edge,
                dir,
                mesh,
                geometry,
                intersectionPreservingVerticesCount,
                edgePolygonSet,
                intersectionFree);
  }
}

void Octree::combine(Octree *a, Octree *b, Topology *g) {

  auto bigger = a;
  auto smaller = b;

  if (b->cluster->size() > a->cluster->size()) {
    bigger = b;
    smaller = a;
  }
  for (auto s : *smaller->cluster) {
    bigger->cluster->insert(s);
  }
  bigger->clusterQef->combine(*smaller->clusterQef);

  vec3 biggerMax = *bigger->clusterSize + *bigger->clusterMin;
  vec3 smallerMax = *smaller->clusterSize + *smaller->clusterMin;

  vec3 combineMax = glm::max(biggerMax, smallerMax);
  vec3 combineMin = glm::min(*bigger->clusterMin, *smaller->clusterMin);
  *bigger->clusterMin = combineMin;
  *bigger->clusterSize = combineMax - combineMin;

//  auto smCluster = smaller->cluster;
//  auto smClusterQef = smaller->clusterQef;
//  auto smClusterMin = smaller->clusterMin;
//  auto smClusterSize = smaller->clusterSize;

  int count = 0;

  assert(bigger->clusterQef->pointCount > 0);

  for (auto oct : *(smaller->cluster)) {
    oct->cluster = bigger->cluster;
    oct->clusterQef = bigger->clusterQef;
    oct->clusterMin = bigger->clusterMin;
    oct->clusterSize = bigger->clusterSize;
    oct->clusterVertex = bigger->clusterVertex;
    count++;
  }
//  delete smCluster;
//  delete smClusterQef;
//  delete smClusterMin;
//  delete smClusterSize;
}

bool Octree::intersectWithBrothers(int cornerDir, Octree *node) {
  for (int i = 0; i < 3; ++i) {
    int edgeIndex = dirRelatedEdge[cornerDir][node->childIndex][i];
    if (edgeIndex >= 0) {
      if (node->cornerSigns[edge_map[edgeIndex][0]] != node->cornerSigns[edge_map[edgeIndex][1]]) {
        return true;
      }
    }
  }
  return true;
}

bool Octree::findFeatureNodes(Octree *node,
                              std::vector<Octree *> &results,
                              const int cornerDir,
                              bool subdivision,
                              const glm::vec3 &edgeP,
                              const glm::vec3 &normal) {
  if (!node) {
    return false;
  }

  if (!intersectWithBrothers(cornerDir, node)) {
    return false;
  }

  //float parallelrity = glm::dot(normalize(edgeP - node->vertex.hermiteP), normal);
  // if (node->isLeaf && node->qef.roughness > 1e-10) {
  results.push_back(node);
  return true;
  // }
  /*
  bool no_features = true;
  if (!node->isLeaf && subdivision) {
    for (int i = 0; i < 8; ++i) {
      auto spreadNodeIndex = planeSpreadingMask[cornerDir][i];
      auto spreadNode = node->children[spreadNodeIndex];
      if (findFeatureNodes(spreadNode, results, cornerDir, true, edgeP, normal)) {
        break;
      }
    }
  }

  int startIndex = 1;
  if (adjacentNodes[cornerDir][node->childIndex] == 1) {
    startIndex = 4;
  } else if (adjacentNodes[7 - cornerDir][node->childIndex] == 1) {
    startIndex = 7;
  } else if (cornerDir + node->childIndex == 7) {
    startIndex = 8;
  }
  for (int i = startIndex; i < 8; ++i) {
    auto spreadNodeIndex = planeSpreadingMask[cornerDir][i];
    auto spreadNode = node->parent->children[spreadNodeIndex];
    assert(spreadNode != node);
    if (!spreadNode) {
      break;
    }
    if (findFeatureNodes(spreadNode, results, cornerDir, true, edgeP, normal)) {
      no_features = false;
      break;
    }
  }
  if (no_features && !subdivision) {
    return findFeatureNodes(node->parent, results, cornerDir, false, edgeP, normal);
  }
  return true;
   */
}

void calVertex(Vertex *v, vec3 p, Mesh *mesh, Topology *g) {
  v->hermiteP = p;
  g->normal(p, v->hermiteN);
  v->vertexIndex = static_cast<unsigned int>(mesh->positions.size());
  mesh->positions.push_back(v->hermiteP);
  mesh->normals.push_back(v->hermiteN);
}

bool Octree::isInterFreeCondition2Faild(const std::vector<Vertex *> &polygons, const vec3 &p1, const vec3 &p2) {
  int anotherV = 3;
  bool interSupportingEdge = false;

  for (int i = 2; i < polygons.size(); ++i) {
    vec3 baryPos;
    bool isInter = glm::intersectRayTriangle(p1,
                                             p2 - p1,
                                             polygons[0]->hermiteP,
                                             polygons[i - 1]->hermiteP,
                                             polygons[i]->hermiteP,
                                             baryPos);
    isInter = isInter && (baryPos.z > 0.f && baryPos.z < 1.f);
    if (isInter) {
      interSupportingEdge = true;
      anotherV = i % 3 + 1;
    }
  }
  if (polygons.size() == 3) {
    return !interSupportingEdge;
  } else {
    vec3 baryPos;
    bool interTetrahedron = glm::intersectRayTriangle(polygons[0]->hermiteP,
                                                      polygons[2]->hermiteP - polygons[0]->hermiteP,
                                                      p1,
                                                      p2,
                                                      polygons[anotherV]->hermiteP,
                                                      baryPos);
    interTetrahedron = interTetrahedron && (baryPos.z > 0.f && baryPos.z < 1.f);
    return !(interTetrahedron && interSupportingEdge);
  }
}

void Octree::generateQuad(Octree *nodes[4],
                          int dir,
                          Mesh *mesh,
                          Topology *g,
                          int &intersectionPreservingVerticesCount,
                          EdgePolygonSet &edgePolygonSet,
                          bool intersectionFree) {
  std::unordered_set<Vertex *> identifier;
  std::vector<Vertex *> polygon;
  bool condition1Failed = false;
  for (int i = 0; i < 4; ++i) {
    if (identifier.find(nodes[i]->clusterVertex) == identifier.end()) {
      polygon.push_back(nodes[i]->clusterVertex);
    }
    identifier.insert(nodes[i]->clusterVertex);
  }

  if (polygon.size() < 3) {
    return;
  }
  int sameCellIndex[2] = {2, 3};
  int firstConcaveFaceVertex = 0;
  for (int i = 0; i < 4; ++i) {
    int testDir = (dir + i / 2 + 1) % 3;
    int edgeAdjacentCellIndexA = edgeTestNodeOrder[i][0];
    int edgeAdjacentCellIndexB = edgeTestNodeOrder[i][1];
    Octree *a = nodes[edgeAdjacentCellIndexA];
    Octree *b = nodes[edgeAdjacentCellIndexB];
    if (a->cluster != b->cluster) {
      if (a->faceVertices.find(b) != a->faceVertices.end()) {
        firstConcaveFaceVertex = i;
        condition1Failed = true;
        continue;
      }
      vec3 faceMinA = vec3(std::numeric_limits<float>::max());
      vec3 faceMinB = faceMinA;
      vec3 faceMaxA = -vec3(std::numeric_limits<float>::max());
      vec3 faceMaxB = faceMaxA;
      for (int j = 0; j < 4; ++j) {
        int subIndexA = faceProcFaceMask[testDir][j][0];
        int subIndexB = faceProcFaceMask[testDir][j][1];
        vec3 cornerA = min_offset_subdivision(subIndexA) * (*a->clusterSize) + *a->clusterMin;
        vec3 cornerB = min_offset_subdivision(subIndexB) * (*b->clusterSize) + *b->clusterMin;
        faceMinA = glm::min(cornerA, faceMinA);
        faceMinB = glm::min(cornerB, faceMinB);
        faceMaxA = glm::max(cornerA, faceMaxA);
        faceMaxB = glm::max(cornerB, faceMaxB);
      }
      vec3 faceMin = glm::max(faceMinA, faceMinB);
      vec3 faceMax = glm::min(faceMaxA, faceMaxB);
      if (!segmentFaceIntersection(a->clusterVertex->hermiteP, b->clusterVertex->hermiteP, faceMin, faceMax, testDir)) {
        vec3 minEnd = faceMin + directionMap(dir) * (faceMax - faceMin);
        vec3 maxEnd = faceMax - directionMap(dir) * (faceMax - faceMin);
        glm::vec3 points[4] = {faceMin, minEnd, faceMax, maxEnd};
        vec3 massPointSum(0.f);
        int pointCount = 0;
        for (int k = 0; k < 4; ++k) {
          float v1 = g->value(points[k]);
          float v2 = g->value(points[(k + 1) % 4]);
          if ((v1 >= 0 && v2 < 0) || (v1 < 0 && v2 >= 0)) {
            vec3 x;
            g->solve(points[k], points[(k + 1) % 4], x);
            massPointSum += x;
            pointCount++;
          }
        }
        if (pointCount > 0) {
          firstConcaveFaceVertex = i;
          auto faceV = new Vertex();
          calVertex(faceV, massPointSum / (float) pointCount, mesh, g);
          a->faceVertices[b] = faceV;
          b->faceVertices[a] = faceV;
          condition1Failed = true;
        }
      }
    } else {
      sameCellIndex[0] = edgeAdjacentCellIndexA;
      sameCellIndex[1] = edgeAdjacentCellIndexB;
    }
  }

  int minCellIndex = 0;
  for (int i = 0; i < 4; ++i) {
    int edgeAdjacentCellIndexA = edgeTestNodeOrder[i][0];
    int edgeAdjacentCellIndexB = edgeTestNodeOrder[i][1];
    if (edgeAdjacentCellIndexA != sameCellIndex[0] && edgeAdjacentCellIndexA != sameCellIndex[1]
        && edgeAdjacentCellIndexB != sameCellIndex[0] && edgeAdjacentCellIndexB != sameCellIndex[1]) {
      minCellIndex = edgeAdjacentCellIndexA;
    }
  }

  Vertex edgeVertex;
  vec3 p1 = *nodes[minCellIndex]->clusterSize * min_offset_subdivision(edgeProcEdgeMask[dir][0][minCellIndex])
      + *nodes[minCellIndex]->clusterMin;
  vec3 p2 = *nodes[minCellIndex]->clusterSize * min_offset_subdivision(edgeProcEdgeMask[dir][1][minCellIndex])
      + *nodes[minCellIndex]->clusterMin;

  for (int i = 0; i < 4; ++i) {
    p1[dir] = std::max((*nodes[i]->clusterMin)[dir], p1[dir]);
    p2[dir] = std::min((*nodes[i]->clusterMin)[dir] + (*nodes[i]->clusterSize)[dir], p2[dir]);
  }

  float v1 = g->value(p1);
  float v2 = g->value(p2);
  if ((v1 >= 0 && v2 >= 0) || (v1 < 0 && v2 < 0)) {
    return;
  }

  std::set<Vertex*> polygonSet;

  for (auto v : polygon) {
    polygonSet.insert(v);
  }

  if (edgePolygonSet.find(polygonSet) != edgePolygonSet.end()) {
    return;
  }

  edgePolygonSet.insert(polygonSet);

  bool condition2Failed = isInterFreeCondition2Faild(polygon, p1, p2);
  if (polygon.size() > 3) {
    std::vector<Vertex *> reversePolygons = {polygon[1], polygon[2], polygon[3], polygon[0]};
    bool reverseCondition2Failed = isInterFreeCondition2Faild(reversePolygons, p1, p2);
    if (!reverseCondition2Failed) {
      /// NOTE: the swap here happens whether intersection-free or not
      polygon.swap(reversePolygons);
    }
    condition2Failed = condition2Failed && reverseCondition2Failed;
  }
  if (intersectionFree && (condition1Failed || condition2Failed)) {
    polygon.clear();
    if (!condition2Failed) {
      std::vector<int> concaveFlags;
      std::vector<Vertex *> convexPart;
      // edge vertex
      intersectionPreservingVerticesCount++;
      int concaveCount = 0;
      for (int i = 0; i < 4; ++i) {
        int index = (i + firstConcaveFaceVertex) % 4;
        auto faceIter = nodes[index]->faceVertices.find(nodes[(index + 1) % 4]);
        auto cellVertex = nodes[(index + 1) % 4]->clusterVertex;
        if (faceIter != nodes[index]->faceVertices.end()) {
          polygon.push_back(faceIter->second);
          concaveFlags.push_back(1);
          convexPart.push_back(faceIter->second);
          concaveCount++;
        }
        polygon.push_back(cellVertex);
        concaveFlags.push_back(0);
      }
      for (int i = 0; i < polygon.size() - 2; ++i) {
        Vertex *triangle[3] = {
            polygon[0], polygon[i + 1], polygon[i + 2]
        };
        detectSharpTriangles(triangle, mesh, g);
      }
    } else {
      g->solve(p1, p2, edgeVertex.hermiteP);
      calVertex(&edgeVertex, edgeVertex.hermiteP, mesh, g);
      for (int i = 0; i < 4; ++i) {
        Octree *a = nodes[i];
        Octree *b = nodes[(i + 1) % 4];
        if (a != b) {
          polygon.push_back(a->clusterVertex);
          auto faceVIter = a->faceVertices.find(b);
          if (faceVIter != a->faceVertices.end()) {
            // face vertex
            intersectionPreservingVerticesCount++;
            polygon.push_back(faceVIter->second);
            polygon.push_back(faceVIter->second);
          }
          polygon.push_back(b->clusterVertex);
        }
      }
      for (int i = 0; i < polygon.size() / 2; ++i) {
        Vertex *triangle[3] = {
            &edgeVertex, polygon[i * 2], polygon[i * 2 + 1]
        };
        detectSharpTriangles(triangle, mesh, g);
      }
    }
  } else {
    for (int i = 2; i < polygon.size(); ++i) {
      Vertex *triangle[3] = {
          polygon[0], polygon[i - 1], polygon[i]
      };
      detectSharpTriangles(triangle, mesh, g);
    }
  }
}

void Octree::generatePolygons(Octree *nodes[4], int dir, Mesh *mesh, Topology *g) {
  int minNodeIndex = -1;
  int maxDepth = -1;
  for (int i = 0; i < 4; ++i) {
    if (nodes[i]->depth > maxDepth) {
      minNodeIndex = i;
      maxDepth = nodes[i]->depth;
    }
  }
  int edgeIndex = processEdgeMask[dir][minNodeIndex];

  glm::vec3 p1 = *nodes[minNodeIndex]->clusterMin
      + min_offset_subdivision(edge_map[edgeIndex][0]) * (*nodes[minNodeIndex]->clusterSize);
  glm::vec3 p2 = *nodes[minNodeIndex]->clusterMin
      + min_offset_subdivision(edge_map[edgeIndex][1]) * (*nodes[minNodeIndex]->clusterSize);
  // uint8_t corner1 = nodes[minNodeIndex]->cornerSigns[edge_map[edgeIndex][0]];
  uint8_t corner2 = nodes[minNodeIndex]->cornerSigns[edge_map[edgeIndex][1]];
  glm::vec3 edgeP, normal;
  g->solve(p1, p2, edgeP);
  g->normal(edgeP, normal);
  std::vector<Octree *> polygons;

  for (int i = 0; i < 4; ++i) {
    float NDotDir = glm::dot(edgeProcDir(dir, i), normal);
    int dirSign = 1;
    if (NDotDir > 0.f && corner2 == 0) {
      dirSign = 0;
    }
    int cornerDir = planeSpreadingDir[dir][dirSign][i];
    findFeatureNodes(nodes[i],
                     polygons,
                     cornerDir,
                     false,
                     edgeP,
                     normal);
  }
  if (polygons.size() > 2) {
    for (int i = 2; i < polygons.size(); ++i) {
      mesh->indices.push_back(polygons[0]->vertex.vertexIndex);
      mesh->indices.push_back(polygons[i - 1]->vertex.vertexIndex);
      mesh->indices.push_back(polygons[i]->vertex.vertexIndex);
    }
  }
}

void Octree::detectSharpTriangles(Vertex *vertices[3], Mesh *mesh, Topology *g) {
  for (int j = 0; j < 3; ++j) {
    auto targetVert = vertices[j];
    Vertex *adjacentVerts[2] = {vertices[(j + 1) % 3], vertices[(j + 2) % 3]};
    glm::vec3 offset =
        adjacentVerts[1]->hermiteP - targetVert->hermiteP +
            adjacentVerts[0]->hermiteP - targetVert->hermiteP;
    offset *= 0.05f;
    glm::vec3 normal;
    g->normal(targetVert->hermiteP + offset, normal);
    if (glm::dot(normal, targetVert->hermiteN) < std::cos(glm::radians(15.f))) {
      mesh->indices.push_back(static_cast<unsigned int>(mesh->positions.size()));
      mesh->positions.push_back(targetVert->hermiteP);
      mesh->normals.push_back(normal);
    } else {
      mesh->indices.push_back(targetVert->vertexIndex);
    }
  }
}

void Octree::drawOctrees(Octree *root, Mesh *mesh, std::unordered_set<Vertex *> &visited) {
  if (!root) {
    return;
  }
  if (root->isLeaf) {
    if (visited.find(root->clusterVertex) != visited.end()) {
      return;
    }
    visited.insert(root->clusterVertex);
    for (int i = 0; i < 12; ++i) {
      auto a = min_offset_subdivision(cellProcFaceMask[i][0]) * (*root->clusterSize) + (*root->clusterMin);
      auto b = min_offset_subdivision(cellProcFaceMask[i][1]) * (*root->clusterSize) + (*root->clusterMin);
//      auto a = min_offset_subdivision(cellProcFaceMask[i][0]) * (root->clusterSize) + (root->min);
//      auto b = min_offset_subdivision(cellProcFaceMask[i][1]) * (root->size) + (root->min);



      auto na = normalize(min_offset_subdivision(cellProcFaceMask[i][0]) - vec3(0.5f));
      auto nb = normalize(min_offset_subdivision(cellProcFaceMask[i][1]) - vec3(0.5f));

      mesh->positions.push_back(a);
      mesh->positions.push_back(a);
      mesh->positions.push_back(b);
      mesh->normals.push_back(na);
      mesh->normals.push_back(na);
      mesh->normals.push_back(nb);
      mesh->indices.push_back(static_cast<unsigned int &&>(mesh->indices.size()));
      mesh->indices.push_back(static_cast<unsigned int &&>(mesh->indices.size()));
      mesh->indices.push_back(static_cast<unsigned int &&>(mesh->indices.size()));
    }
    return;
  }
  for (int i = 0; i < 8; ++i) {
    drawOctrees(root->children[i], mesh, visited);
  }
}

void Octree::calHermite(Octree *node, QefSolver *qef, Topology *g, Vertex *vertex) {
  auto &p = vertex->hermiteP;
  qef->solve(p, node->error);
  const auto min = *node->clusterMin - vec3(1e-6);
  const auto max = *node->clusterMin + (*node->clusterSize) + vec3(1e-6);
  if (p.x < min.x || p.x > max.x ||
      p.y < min.y || p.y > max.y ||
      p.z < min.z || p.z > max.z) {

    p = qef->massPointSum / (float) qef->pointCount;
    if (p.x < min.x || p.x > max.x ||
        p.y < min.y || p.y > max.y ||
        p.z < min.z || p.z > max.z) {
      p = glm::min(*node->clusterMin + (*node->clusterSize), p);
      p = glm::max(*node->clusterMin, p);
    }
  }

  g->normal(p, vertex->hermiteN);
}