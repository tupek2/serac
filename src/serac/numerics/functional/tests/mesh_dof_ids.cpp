#include "mfem.hpp"

#include <iostream>

enum class Family { H1, Hcurl, DG };

mfem::FiniteElementSpace * makeFiniteElementSpace(Family family, int order, mfem::Mesh & mesh)
{
  const int                      dim = mesh.Dimension();
  mfem::FiniteElementCollection* fec;
  const auto                     ordering = mfem::Ordering::byVDIM;

  switch (family) {
    case Family::H1:
      fec = new mfem::H1_FECollection(order, dim);
      break;
    case Family::Hcurl:
      fec = new mfem::ND_FECollection(order, dim);
      break;
    case Family::DG:
      fec = new mfem::L2_FECollection(order, dim);
      break;
    default:
      return NULL;
      break;
  }

  // note: this leaks memory: `fec` is never destroyed, but how to fix?
  return new mfem::FiniteElementSpace(&mesh, fec, 1, ordering);
}

mfem::Mesh patch_test_mesh(mfem::Geometry::Type geom) {
    mfem::Mesh output;
    switch (geom) {
        case mfem::Geometry::Type::TRIANGLE: 
            output = mfem::Mesh(2, 5, 4);
            output.AddVertex(0.0, 0.0);
            output.AddVertex(1.0, 0.0);
            output.AddVertex(1.0, 1.0);
            output.AddVertex(0.0, 1.0);
            output.AddVertex(0.7, 0.4);

            output.AddTriangle(0, 1, 4);
            output.AddTriangle(1, 2, 4);
            output.AddTriangle(2, 3, 4);
            output.AddTriangle(3, 0, 4);
            break;
        case mfem::Geometry::Type::SQUARE:
            output = mfem::Mesh(2, 8, 5);
            output.AddVertex(0.0, 0.0);
            output.AddVertex(1.0, 0.0);
            output.AddVertex(1.0, 1.0);
            output.AddVertex(0.0, 1.0);
            output.AddVertex(0.2, 0.3);
            output.AddVertex(0.6, 0.3);
            output.AddVertex(0.7, 0.8);
            output.AddVertex(0.4, 0.7);

            output.AddQuad(0, 1, 5, 4);
            output.AddQuad(1, 2, 6, 5);
            output.AddQuad(2, 3, 7, 6);
            output.AddQuad(3, 0, 4, 7);
            output.AddQuad(4, 5, 6, 7);
            break;
        case mfem::Geometry::Type::TETRAHEDRON:
            output = mfem::Mesh(3, 9, 12);
            output.AddVertex(0.0, 0.0, 0.0);
            output.AddVertex(1.0, 0.0, 0.0);
            output.AddVertex(1.0, 1.0, 0.0);
            output.AddVertex(0.0, 1.0, 0.0);
            output.AddVertex(0.0, 0.0, 1.0);
            output.AddVertex(1.0, 0.0, 1.0);
            output.AddVertex(1.0, 1.0, 1.0);
            output.AddVertex(0.0, 1.0, 1.0);
            output.AddVertex(0.4, 0.6, 0.7);

            output.AddTet(0, 1, 2, 8); 
            output.AddTet(0, 2, 3, 8); 
            output.AddTet(4, 5, 1, 8); 
            output.AddTet(4, 1, 0, 8); 
            output.AddTet(5, 6, 2, 8); 
            output.AddTet(5, 2, 1, 8); 
            output.AddTet(6, 7, 3, 8); 
            output.AddTet(6, 3, 2, 8); 
            output.AddTet(7, 4, 0, 8); 
            output.AddTet(7, 0, 3, 8); 
            output.AddTet(7, 6, 5, 8); 
            output.AddTet(7, 5, 4, 8);
            break;
        case mfem::Geometry::Type::CUBE:
            output = mfem::Mesh(3, 16, 7);
            output.AddVertex(0.0, 0.0, 0.0); 
            output.AddVertex(1.0, 0.0, 0.0); 
            output.AddVertex(1.0, 1.0, 0.0); 
            output.AddVertex(0.0, 1.0, 0.0); 
            output.AddVertex(0.0, 0.0, 1.0); 
            output.AddVertex(1.0, 0.0, 1.0); 
            output.AddVertex(1.0, 1.0, 1.0); 
            output.AddVertex(0.0, 1.0, 1.0); 
            output.AddVertex(0.2, 0.3, 0.3); 
            output.AddVertex(0.7, 0.5, 0.3); 
            output.AddVertex(0.7, 0.7, 0.3); 
            output.AddVertex(0.3, 0.8, 0.3); 
            output.AddVertex(0.3, 0.4, 0.7); 
            output.AddVertex(0.7, 0.2, 0.6); 
            output.AddVertex(0.7, 0.6, 0.7); 
            output.AddVertex(0.2, 0.7, 0.6);

            output.AddHex( 0,  1,  2,  3,  8,  9, 10, 11);
            output.AddHex( 4,  5,  1,  0, 12, 13,  9,  8);
            output.AddHex( 5,  6,  2,  1, 13, 14, 10,  9);
            output.AddHex( 6,  7,  3,  2, 14, 15, 11, 10);
            output.AddHex( 7,  4,  0,  3, 15, 12,  8, 11);
            output.AddHex(12, 13, 14, 15,  4,  5,  6,  7);
            output.AddHex( 8,  9, 10, 11, 12, 13, 14, 15);
            break;
        
        default:
            std::cout << "patch_test_mesh(): unsupported geometry type" << std::endl;
            exit(1);
            break;
    }
    output.FinalizeMesh();
    return output;
}

std::string to_string(Family f) {
    if (f == Family::H1) return "H1";
    if (f == Family::Hcurl) return "Hcurl";
    if (f == Family::DG) return "DG";
    return "";
}

std::string to_string(mfem::Geometry::Type geom) {
    if (geom == mfem::Geometry::Type::TRIANGLE) return "Triangle";
    if (geom == mfem::Geometry::Type::TETRAHEDRON) return "Tetrahedron";
    if (geom == mfem::Geometry::Type::SQUARE) return "Quadrilateral";
    if (geom == mfem::Geometry::Type::CUBE) return "Hexahedron";
    return "";
}

mfem::Geometry::Type face_type(mfem::Geometry::Type geom) {
    if (geom == mfem::Geometry::Type::TRIANGLE) { return mfem::Geometry::Type::SEGMENT; }
    if (geom == mfem::Geometry::Type::SQUARE) {  return mfem::Geometry::Type::SEGMENT; } 
    if (geom == mfem::Geometry::Type::TETRAHEDRON) {  return mfem::Geometry::Type::TRIANGLE; } 
    if (geom == mfem::Geometry::Type::CUBE) { return mfem::Geometry::Type::SQUARE; }
    return mfem::Geometry::Type::INVALID;
}

bool isH1(const mfem::FiniteElementSpace& fes) {
  return (fes.FEColl()->GetContType() == mfem::FiniteElementCollection::CONTINUOUS);
}

bool isHcurl(const mfem::FiniteElementSpace& fes) {
  return (fes.FEColl()->GetContType() == mfem::FiniteElementCollection::TANGENTIAL);
}

bool isDG(const mfem::FiniteElementSpace& fes) {
  return (fes.FEColl()->GetContType() == mfem::FiniteElementCollection::DISCONTINUOUS);
}

struct DoF {
    uint64_t sign : 1;
    uint64_t orientation : 4;
    uint64_t index : 48;
};

template < typename T >
struct Range{
    T * begin() { return ptr[0]; }
    T * end() { return ptr[1]; }
    T * ptr[2];
};

template < typename T >
struct Array2D {
    Array2D() : values{}, dim{} {};
    Array2D(uint64_t m, uint64_t n) : values(m * n, 0), dim{m, n} {};
    Array2D(std::vector<T> && data, uint64_t m, uint64_t n) : values(data), dim{m, n} {};
    Range<T> operator()(int i) { return Range<T>{&values[i * dim[0]], &values[(i + 1) * dim[0]]}; }
    T & operator()(int i, int j) { return values[i * dim[0] + j]; }
    std::vector < T > values;
    uint64_t dim[2];
};

std::vector< Array2D<int> > geom_local_face_dofs(int p) {

    // FullSimplify[InterpolatingPolynomial[{
    //   {{0, 2}, (p + 1) + p},
    //   {{0, 1}, p + 1}, {{1, 1}, p + 2},
    //   {{0, 0}, 0}, {{1, 0}, 1}, {{2, 0}, 2}
    // }, {x, y}]]
    // 
    // x + 1/2 (3 + 2 p - y) y
    auto tri_id = [p](int x, int y) { return x + ((3 + 2 * p - y) * y) / 2; };

    // FullSimplify[InterpolatingPolynomial[{
    //  {{0, 3}, ((p - 1) (p) + (p) (p + 1) + (p + 1) (p + 2))/2},
    //  {{0, 2}, ((p) (p + 1) + (p + 1) (p + 2))/2}, {{1, 2},  p - 1 + ((p) (p + 1) + (p + 1) (p + 2))/2},
    //  {{0, 1}, (p + 1) (p + 2)/2}, {{1, 1}, p + (p + 1) (p + 2)/2}, {{2, 1}, 2 p - 1 + (p + 1) (p + 2)/2},
    //  {{0, 0}, 0}, {{1, 0}, p + 1}, {{2, 0}, 2 p + 1}, {{3, 0}, 3 p}
    // }, {y, z}]] + x
    // 
    // x + (z (11 + p (12 + 3 p) - 6 y + z (z - 6 - 3 p)) - 3 y (y - 2 p - 3))/6  
    auto tet_id = [p](int x, int y, int z) { 
        return x + (z*(11+p*(12+3*p) - 6*y + z*(z-6-3*p)) - 3*y*(y-2*p-3)) / 6;
    };

    auto quad_id = [p](int x, int y) { return ((p+1) * y) + x; };

    auto hex_id = [p](int x, int y, int z) { return (p+1) * ((p+1) * z + y) + x; };

    std::vector<Array2D< int > > output(mfem::Geometry::Type::NUM_GEOMETRIES);

    Array2D<int> tris(3, p+1);
    for (int k = 0; k <= p; k++) {
        tris(0, k) = tri_id(k, 0);
        tris(1, k) = tri_id(p-k, k);
        tris(2, k) = tri_id(0, p-k);
    }
    output[mfem::Geometry::Type::TRIANGLE] = tris;

    Array2D<int> quads(4, p+1);
    for (int k = 0; k <= p; k++) {
        quads(0, k) = quad_id(k, 0);
        quads(1, k) = quad_id(p, k);
        quads(2, k) = quad_id(p - k, k);
        quads(3, k) = quad_id(0, p - k);
    }
    output[mfem::Geometry::Type::SQUARE] = quads;

    // Vertices for Geometry::TETRAHEDRON
    // {{0.0 0.0 0.0}, {1.0 0.0 0.0}, {0.0 1.0 0.0}, {0.0 0.0 1.0}}
    //
    // Faces for Geometry::TETRAHEDRON
    // {{1, 2, 3}, {0, 3, 2}, {0, 1, 3}, {0, 2, 1}};
    Array2D<int> tets(4, (p+1) * (p+2) / 2);
    for (int k = 0; k <= p; k++) {
        for (int j = 0; j <= p - k; j++) {
            tets(0, k) = tet_id(0, 0, 0);
            tets(1, k) = tet_id(0, 0, 0);
            tets(2, k) = tet_id(0, 0, 0);
            tets(3, k) = tet_id(0, 0, 0);
        }
    }
    output[mfem::Geometry::Type::TETRAHEDRON] = tets;

    // Vertices for Geometry::CUBE
    // {0.0 0.0 0.0}, {1.0 0.0 0.0}, {1.0 1.0 0.0}, {0.0 1.0 0.0},
    // {0.0 0.0 1.0}, {1.0 0.0 1.0}, {1.0 1.0 1.0}, {0.0 1.0 1.0}
    //
    // Faces for Geometry::CUBE
    //    {3, 2, 1, 0}, {0, 1, 5, 4}, {1, 2, 6, 5},
    //    {2, 3, 7, 6}, {3, 0, 4, 7}, {4, 5, 6, 7}
    // };
    Array2D<int> hexes(6, (p+1) * (p+1));
    for (int k = 0; k <= p; k++) {
        for (int j = 0; j <= p; j++) {
            hexes(0, k*(p+1)+j) = hex_id(p-j, p-k, 0);
            hexes(1, k*(p+1)+j) = hex_id(0, 0, 0);
            hexes(2, k*(p+1)+j) = hex_id(0, 0, 0);
            hexes(3, k*(p+1)+j) = hex_id(0, 0, 0);
            hexes(4, k*(p+1)+j) = hex_id(0, 0, 0);
            hexes(5, k*(p+1)+j) = hex_id(0, 0, 0);
        }
    }
    output[mfem::Geometry::Type::CUBE] = hexes;

    return output;

}

Array2D< int > GetBoundaryFaceDofs(mfem::FiniteElementSpace * fes, mfem::Geometry::Type face_geom) {

    std::vector < int > face_dofs;
    mfem::Mesh * mesh = fes->GetMesh();
    mfem::Table * face_to_elem = mesh->GetFaceToElementTable();
    std::vector< Array2D<int> > local_face_dofs = geom_local_face_dofs(fes->GetElementOrder(0));
    // note: this assumes that all the elements are the same polynomial order ^^^

    uint64_t n = 0;

    for (int f = 0; f < fes->GetNF(); f++) {

        // don't bother with interior faces, or faces with the wrong geometry
        if (mesh->FaceIsInterior(f) || mesh->GetFaceGeometryType(f) != face_geom) {
          continue;
        }

        // mfem doesn't provide this connectivity info for DG spaces directly,
        // so we have to get at it indirectly in several steps:
        if (isDG(*fes)) {
        
            // 1. find the element that this face belongs to
            mfem::Array<int> elem_ids;
            face_to_elem->GetRow(f, elem_ids);

            // 2. find `i` such that `elem_side_ids[i] == f`
            mfem::Array<int> elem_side_ids, orientations;
            if (mesh->Dimension() == 2) {
                mesh->GetElementEdges(elem_ids[0], elem_side_ids, orientations);
            } else {
                mesh->GetElementFaces(elem_ids[0], elem_side_ids, orientations);
            }

            int i;
            for (i = 0; i < elem_side_ids.Size(); i++) {
                if (elem_side_ids[i] == f) break;
            }

            std::cout << orientations[i] << std::endl;

            // 3. get the dofs for the entire element
            mfem::Array<int> elem_dof_ids;
            fes->GetElementVDofs(elem_ids[0], elem_dof_ids);

            // 4. extract only the dofs that correspond to side `i`
            mfem::Geometry::Type elem_geom = mesh->GetElementGeometry(elem_ids[0]);
            for (auto k : local_face_dofs[elem_geom](i)) {
                face_dofs.push_back(elem_dof_ids[k]);
            }

        // H1 and Hcurl spaces are more straight-forward, since
        // we can use FiniteElementSpace::GetFaceVDofs() directly
        } else {

            mfem::Array<int> vdofs;
            fes->GetFaceVDofs(f, vdofs);
            for (auto dof : vdofs) {
                face_dofs.push_back(dof);
            }

        }

        n++;

    }

    delete face_to_elem;

    uint64_t dofs_per_face = face_dofs.size() / n;

    return Array2D<int>(std::move(face_dofs), n, dofs_per_face);

}

//#define ENABLE_GLVIS

int main() {

    Family families[] = {Family::H1, Family::Hcurl, Family::DG};
    mfem::Geometry::Type geometries[] = {
        mfem::Geometry::Type::TRIANGLE, 
        mfem::Geometry::Type::SQUARE, 
        mfem::Geometry::Type::TETRAHEDRON, 
        mfem::Geometry::Type::CUBE};

    #if defined ENABLE_GLVIS
    char vishost[] = "localhost";
    int  visport   = 19916;
    #endif

    for (auto geom : geometries) {

        mfem::Mesh mesh = patch_test_mesh(geom);

        #if defined ENABLE_GLVIS
        mfem::socketstream sol_sock(vishost, visport);
        sol_sock.precision(8);
        sol_sock << "mesh\n" << mesh << std::flush;
        #endif

        for (auto family : families) {

            std::cout << to_string(geom) << " " << to_string(family);

            mfem::FiniteElementSpace * fes = makeFiniteElementSpace(family, 3, mesh);
            
            std::cout << ", " << fes->GetNDofs() << " " << fes->GetNVDofs() << std::endl;

            std::cout << "volume elements: " << std::endl;
            for (int i = 0; i < fes->GetNE(); i++) {
                mfem::Array<int> vdofs;
                fes->GetElementVDofs(i, vdofs);
                vdofs.Print(std::cout, 64);
            }

            std::cout << "face elements: " << fes->GetNF() << std::endl;
            std::cout << "bdr elements: " << fes->GetNBE() << std::endl;
            GetBoundaryFaceDofs(fes, face_type(geom));

        }

    }

}
