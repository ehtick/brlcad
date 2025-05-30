find_package_eigen(REQUIRED)
find_package_gte(REQUIRED)

set(
  SPSR_srcs
  spsr/Allocator.h
  spsr/Array.h
  spsr/Array.inl
  spsr/BinaryNode.h
  spsr/BSplineData.h
  spsr/BSplineData.inl
  spsr/cvertex.h
  spsr/Factor.cpp
  spsr/Factor.h
  spsr/FunctionData.h
  spsr/FunctionData.inl
  spsr/Geometry.cpp
  spsr/Geometry.h
  spsr/Geometry.inl
  spsr/MarchingCubes.cpp
  spsr/MarchingCubes.h
  spsr/MAT.h
  spsr/MAT.inl
  spsr/MultiGridOctreeData.h
  spsr/MultiGridOctreeData.inl
  spsr/MultiGridOctreeData.IsoSurface.inl
  spsr/MultiGridOctreeData.SortedTreeNodes.inl
  spsr/Octree.h
  spsr/Octree.inl
  spsr/PlyVertexMini.h
  spsr/PointStream.h
  spsr/PointStream.inl
  spsr/Polynomial.h
  spsr/Polynomial.inl
  spsr/PPolynomial.h
  spsr/PPolynomial.inl
  spsr/SparseMatrix.h
  spsr/SparseMatrix.inl
  spsr/SPSR.cpp
  spsr/SPSR.h
  spsr/Vector.h
  spsr/Vector.inl
)

set(
  LIBBG_SOURCES
  aabb_ray.c
  chull.c
  chull2.cpp
  QuickHull.cpp
  chull3d.cpp
  clip.c
  clipper.cpp
  decimate.cpp
  lseg_lseg.cpp
  lseg_pt.c
  obr.c
  pca.cpp
  plane.cpp
  polygon.c
  polygon_op.cpp
  polygon_triangulate.cpp
  polygon_point_in.c
  sat.c
  spsr.c
  ${SPSR_srcs}
  tri_pt.c
  tri_ray.c
  tri_tri.c
  trimesh.cpp
  trimesh_diff.cpp
  trimesh_isect.cpp
  trimesh_plot3.cpp
  trimesh_sync.cpp
  trimesh_split.cpp
  vert_tree.c
  util.c
)

# Note - libbg_deps is defined by ${BRLCAD_SOURCE_DIR}/src/source_dirs.cmake
set(BG_LIBS ${libbg_deps})

set(BG_INCLUDE_DIRS ${SPSR_INCLUDE_DIR} ${EIGEN3_INCLUDE_DIR} ${GTE_INCLUDE_DIR})

brlcad_addlib(libbg "${LIBBG_SOURCES}" "${BG_LIBS}" "" "${BG_INCLUDE_DIRS}")
set_target_properties(libbg PROPERTIES VERSION 20.0.1 SOVERSION 20)

# altivec vectorization is broken in our bundled version of eigen.
# looks like it was fixed in upstream long time ago, so just disable
# for now.  TODO: remove this / retest after next Eigen upgrade.
set_property(TARGET libbg APPEND PROPERTY COMPILE_DEFINITIONS "EIGEN_DONT_VECTORIZE")
if(TARGET libbg-obj)
  set_property(TARGET libbg-obj APPEND PROPERTY COMPILE_DEFINITIONS "EIGEN_DONT_VECTORIZE")
endif(TARGET libbg-obj)

# With newer GCC, optimized linking of SPSR triggers a compiler warning about a
# new allocation potentially exceeding maximum object size.
if(${BRLCAD_OPTIMIZED} MATCHES "ON")
  check_c_compiler_flag(-Wno-error=alloc-size-larger-than= HAVE_NO_ALLOC_SIZE_LARGER_THAN)
  if(HAVE_NO_ALLOC_SIZE_LARGER_THAN)
    target_link_options(libbg PRIVATE -Wno-error=alloc-size-larger-than=)
  endif(HAVE_NO_ALLOC_SIZE_LARGER_THAN)
endif(${BRLCAD_OPTIMIZED} MATCHES "ON")

add_subdirectory(tests)

set(
  bg_ignore
  QuickHull.hpp
  bg_private.h
  clipper.hpp
  delaunator.hpp
  detria.hpp
  earcut.hpp
  pointgen.c
  spsr/LICENSE
)

cmakefiles(${bg_ignore})
cmakefiles(CMakeLists.txt)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
