#ifndef CGAL_MINKOWSKI_SUM_REDUCED_CONV_H
#define CGAL_MINKOWSKI_SUM_REDUCED_CONV_H

#include <CGAL/Arrangement_2/Arrangement_on_surface_2_global.h>
#include <CGAL/Arrangement_with_history_2.h>
#include <CGAL/Arr_dcel_base.h>         // for Arr_dcel_base, etc
#include <CGAL/Arr_enums.h>
#include <CGAL/Arr_segment_traits_2.h>  // for Arr_segment_traits_2
#include <CGAL/Origin.h>                // for ORIGIN
#include <CGAL/Polygon_2.h>             // for Polygon_2
#include <CGAL/aff_transformation_tags.h>  // for Translation, Rotation
#include <CGAL/assertions.h>            // for CGAL_precondition
#include <CGAL/enum.h>                  // for Comparison_result, etc
#include <CGAL/number_utils.h>          // for to_double, sqrt
#include <CGAL/utils.h>                 // for max, min

#include <CGAL/Minkowski_sum_2/new/aabb/AABB_Collision_detector.h>
#include <CGAL/Minkowski_sum_2/new/Arr_SegmentData_traits.h>  // for state, etc

#include <math.h>                       // for acos
#include <stddef.h>                     // for NULL
#include <iterator>                     // for advance, distance
#include <list>                         // for list, list<>::iterator, etc
#include <queue>                        // for queue
#include <set>                          // for set
#include <utility>                      // for pair
#include <vector>                       // for vector
#include <iostream> // TODO remove

#include <boost/unordered_map.hpp>  // for unordered_map
#include <boost/unordered_set.hpp>  // for unordered_set

namespace CGAL {
namespace internal {

struct Less_than_handle {
    template <typename Type>
    bool operator()(Type s1, Type s2) const {
        return (&(*s1) < &(*s2));
    }
};

template <class HalfedgeBase_>
class Arr_map_halfedge : public HalfedgeBase_ {
public:
    bool visited;
    bool isDegenerate;
    int loopNumber;
};

template <class Traits_,
          class HalfedgeBase_ = Arr_halfedge_base<typename Traits_::X_monotone_curve_2> > class Arr_my_extended_dcel :
    public Arr_dcel_base<Arr_vertex_base<typename Traits_::Point_2>,
    Arr_map_halfedge<HalfedgeBase_>,
    Arr_face_base> {

public:

    template<typename T>
    class rebind {
        typedef typename HalfedgeBase_::template rebind
        <typename T::X_monotone_curve_2> Rebind_halfedge;
        typedef typename Rebind_halfedge::other Halfedge_base;

    public:

        typedef Arr_my_extended_dcel<T, Halfedge_base> other;
    };
};

template <class Kernel_, class Container_>
class Minkowski_sum_by_convolution_lien_2 {

public:

    typedef Kernel_ Kernel;
    typedef CGAL::Polygon_2<Kernel, Container_> Polygon_2;

public:

    // Kernel types:
    typedef typename Kernel::Point_2 Point_2;
    typedef typename Kernel::Vector_2 Vector_2;
    typedef typename Kernel::Direction_2 Direction_2;

    // Kernel functors:
    typedef typename Kernel::Equal_2 Equal_2;
    typedef typename Kernel::Construct_translated_point_2 Translate_point_2;
    typedef typename Kernel::Construct_vector_2 Construct_vector_2;
    typedef typename Kernel::Construct_direction_2 Construct_direction_2;
    typedef typename Kernel::Construct_opposite_line_2 Opposite_line_2;
    typedef typename Kernel::Orientation_2 Compute_orientation_2;
    typedef typename Kernel::Compare_xy_2 Compare_xy_2;
    typedef typename Kernel::Counterclockwise_in_between_2 Ccw_in_between_2;
    typedef typename Kernel::Angle_2 Compute_Angle_2;
    typedef typename Kernel::Compare_x_2 Compare_x_2;
    typedef typename Kernel::Is_vertical_2 Is_vertical_2;
    typedef typename Kernel::Compute_x_2 Compute_x_2;
    typedef typename Kernel::Compute_y_2 Compute_y_2;

    // Polygon-related types:
    typedef typename Polygon_2::Vertex_circulator Vertex_circulator;
    typedef std::pair<Vertex_circulator, unsigned int> Vertex_ref;
    typedef std::pair<Vertex_ref, Vertex_ref> Anchor;
    typedef std::list<Anchor> Anchors_queue;

    // Traits-related types:
    typedef Arr_segment_traits_2<Kernel> Traits_2_A;
    typedef Arr_SegmentData_traits<Traits_2_A> Traits_2;

    typedef typename Traits_2_A::Segment_2 Base_Segment_2;
    typedef typename Traits_2::X_monotone_curve_2 Segment_2;
    typedef std::list<Segment_2> Segments_list;

    typedef Arr_my_extended_dcel<Traits_2> Dcel;

    typedef CGAL::Arrangement_with_history_2<Traits_2, Dcel> Arrangement_history_2;
    typedef typename Arrangement_history_2::Halfedge Halfedge;
    typedef typename Arrangement_history_2::Vertex Vertex;
    typedef typename Arrangement_history_2::Vertex_iterator Vertex_iterator;
    typedef typename Arrangement_history_2::Halfedge_iterator Halfedge_iterator;
    typedef typename Arrangement_history_2::Edge_iterator Edge_iterator;
    typedef typename Arrangement_history_2::Halfedge_handle Halfedge_handle;
    typedef typename Arrangement_history_2::Vertex_handle Vertex_handle;
    typedef typename Arrangement_history_2::Face_iterator Face_iterator;
    typedef typename Arrangement_history_2::Face_handle Face_handle;
    typedef typename Arrangement_history_2::Hole_iterator Hole_iterator;
    typedef typename Arrangement_history_2::Halfedge_around_vertex_circulator Halfedge_around_vertex_circulator;
    typedef typename Arrangement_history_2::Ccb_halfedge_circulator Ccb_halfedge_circulator;

    typedef typename Arrangement_history_2::Originating_curve_iterator Originating_curve_iterator;
    typedef std::pair<int, int> StatePair;

    typedef std::set<Halfedge_handle, Less_than_handle> Edges_set;
    typedef std::set<Face_handle, Less_than_handle> Faces_set;

    // Data members:
    Equal_2 f_equal;
    Translate_point_2 f_add;
    Construct_vector_2 f_vector;
    Construct_direction_2 f_direction;
    Compute_orientation_2 f_orientation;
    Compare_xy_2 f_compare_xy;
    Ccw_in_between_2 f_ccw_in_between;
    Compute_x_2 f_compute_x;
    Compute_y_2 f_compute_y;

    typename Traits_2::Compare_endpoints_xy_2 f_compare_endpoints_xy;
    typename Traits_2::Compare_y_at_x_2 f_compare_y_at_x;
    typename Traits_2::Compare_x_2 f_compare_x;

    friend class ConvSegMapper;

    struct ConvSegment {
        Halfedge_handle _he;
        ConvSegment(Halfedge_handle &he): _he(he) {}
        ConvSegment() {}
        bool getVisited() const {
            return _he->visited;
        }

        bool getDegenerate() const {
            return _he->isDegenerate;
        }

        int getLoopNum() {
            return _he->loopNumber;
        }

        Vertex_handle getSrc() {
            return (_he->source());
        }

        Vertex_handle getDst() {
            return (_he->target());
        }

        bool operator<(const ConvSegment &rhs) const {
            return Less_than_handle()(_he, rhs._he);
        }

        bool operator==(const ConvSegment &rhs) const {
            return !Less_than_handle()(_he, rhs._he) && !Less_than_handle()(rhs._he, _he);
        }
    };

    struct ConvSegMapper {
        Arrangement_history_2 *_arr;
        Minkowski_sum_by_convolution_lien_2 *_mink;
        ConvSegMapper(Arrangement_history_2 *arr, Minkowski_sum_by_convolution_lien_2 *mink): _arr(arr), _mink(mink) {
        }

        ConvSegment getSegment(const Halfedge_handle &he) {
            return ConvSegment(_mink->getDirAgreeingHalfedge(*_arr, he));
        }

        Direction_2 getConvSegDir(const ConvSegment &seg) const {
            return _mink->getHalfedgeDir(seg._he);
        }

        void markVisited(ConvSegment &convSeg, int id) {
            _mink->setEdgeVisited(*convSeg._he, true, id);
        }

        bool isBBiggerThenAWithReagrdToC(const ConvSegment &a, const ConvSegment &b, const ConvSegment &c) const {
            Direction_2 dir_a = getConvSegDir(a);
            Direction_2 dir_b = getConvSegDir(b);
            Direction_2 dir_c = getConvSegDir(c);
            return _mink->isDirImproving(dir_a, dir_c, dir_b);
        }

        void getNeighbouringSegments(Vertex_handle v, std::list<ConvSegment> &outSegments, std::list<ConvSegment> &inSegmets) {
            std::list<Halfedge_handle> inList, outList;
            _mink->getEdgesFromVertex(*_arr, v, inList, outList);
            typename std::list<Halfedge_iterator>::const_iterator itr;

            for (itr = inList.begin(); itr != inList.end(); ++itr) {
                inSegmets.push_back(getSegment(*itr));
            }

            for (itr = outList.begin(); itr != outList.end(); ++itr) {
                outSegments.push_back(getSegment(*itr));
            }
        }

        static bool getSegVisited(const ConvSegment &seg) {
            return (seg.getVisited() == true || seg.getDegenerate());
        }

        static bool getSegNotVisited(const ConvSegment &seg) {
            return (seg.getVisited() == false && !seg.getDegenerate());
        }

        double getSignedAngle(const ConvSegment &enter, const ConvSegment &exit) {
            return _mink->getSignedAngle(enter._he, exit._he);
        }

        void filterNonVisitedSegments(const std::list<ConvSegment> &inputList, std::list<ConvSegment> &outList, std::list<ConvSegment> &visitedSegmentsList) {
            int out_size = count_if(inputList.begin(), inputList.end(), &ConvSegMapper::getSegNotVisited);
            outList.resize(out_size);
            remove_copy_if(inputList.begin(), inputList.end(), outList.begin(), &ConvSegMapper::getSegVisited);
            visitedSegmentsList.resize(inputList.size() - out_size);
            remove_copy_if(inputList.begin(), inputList.end(), visitedSegmentsList.begin(), &ConvSegMapper::getSegNotVisited);
        }

        struct SegCompare {
            ConvSegment _incomingSeg;
            ConvSegMapper *_mapperInstance;
            SegCompare(ConvSegment incomingSeg, ConvSegMapper *mapperInstance): _incomingSeg(incomingSeg), _mapperInstance(mapperInstance) {}
            bool operator()(ConvSegment a, ConvSegment b) {
                return _mapperInstance->isBBiggerThenAWithReagrdToC(a, b, _incomingSeg);
            }
        };

        ConvSegment getMaximalEdge(std::list<ConvSegment> outgoingSegs, ConvSegment incomingSeg) {
            SegCompare seg_comp(incomingSeg, this);
            return *max_element(outgoingSegs.begin(), outgoingSegs.end(), seg_comp);
        }

        bool checkLoopClosed(const Vertex_handle &v, ConvSegment &startingSeg, int id) {
            Halfedge_handle h;
            bool notFound = _mink->checkOutgoingNotVisited(*_arr, *v, h, id);

            if (h != Halfedge_handle()) {
                startingSeg = getSegment(h);
            }

            return !notFound;
        }

        template<typename T> void fillEdgesSet(T &edges_set) {
            Edge_iterator itr;

            for (itr = _arr->edges_begin(); itr != _arr->edges_end(); ++itr) {
                _mink->setEdgeVisited(*itr, false, -1);

                if (!itr->isDegenerate) {
                    edges_set.insert(getSegment(itr));
                }
            }
        }

        ConvSegment getOuterSegment() {
            Face_iterator startFace = _arr->unbounded_face();
            Halfedge_iterator perimiterFace = *(startFace -> holes_begin());
            return getSegment(perimiterFace);
        }

        void removeSegFromArr(const ConvSegment &seg) {
            _arr->remove_edge(seg._he);
        }

        void removeRangeFromArr(std::list<ConvSegment> &segsToRemove) {
            for (typename std::list<ConvSegment>::iterator itr = segsToRemove.begin(); itr != segsToRemove.end(); ++itr) {
                removeSegFromArr(*itr);
            }
        }
    };

    friend class DegenerateCasesManager;
    struct DegenerateCasesManager {
        DegenerateCasesManager(Arrangement_history_2 *arr, Minkowski_sum_by_convolution_lien_2 *mink, Polygon_2 *poly1, Polygon_2 *poly2, bool isActive): _arr(arr), _mink(mink), _poly1(poly1), _poly2(poly2), _active(isActive) {
        }

        void markDegenerateEdges() {
            Edge_iterator itr = _arr->edges_begin();

            for (; itr != _arr->edges_end(); ++itr) {
                _mink->setEdgeVisited(*itr, false, -1);

                if (_active) {
                    if (_mink->checkDegenerateEdgeOppositeSegments(*_arr, itr)) {
                        if (!_mink->checkSegmentCollisionDetection(*_arr, itr->curve(), *_poly1, *_poly2)) {
                            _mink->setEdgeDegenerate(*itr, true);
                        } else {
                            _mink->setEdgeDegenerate(*itr, false);
                        }
                    } else {
                        _mink->setEdgeDegenerate(*itr, false);
                    }
                } else {
                    _mink->setEdgeDegenerate(*itr, false);
                }
            }
        }

        void findDegenerateBorderVertices() {
            if (!_active) {
                return;
            }

            Vertex_iterator itr = _arr->vertices_begin();

            for (; itr != _arr->vertices_end(); ++itr) {
                if (_mink->checkDegenarateVertexIsIntersectionOfThreeSegments(*_arr, itr))
                {
                    Point_2 p_end = itr->point();

                    if (!_mink->checkCollisionDetection(*_arr, itr->point(), *_poly1, *_poly2)) {
                        _degenerate_points_list.push_back(itr->point());
                    }
                }
            }
        }

        void addDegenerateVerticesToArr() {
            typename std::list<Point_2>::iterator itr = _degenerate_points_list.begin();

            for (; itr != _degenerate_points_list.end(); ++itr) {
                CGAL::insert_point(*_arr, *itr);
            }
        }

    private:
        Arrangement_history_2 *_arr;
        Minkowski_sum_by_convolution_lien_2 *_mink;
        std::list<Point_2> _degenerate_points_list;
        Polygon_2 *_poly1;
        Polygon_2 *_poly2;
        bool _active;
    };

public:

    Minkowski_sum_by_convolution_lien_2() {
        // Obtain kernel functors.
        Kernel ker;

        f_equal = ker.equal_2_object();
        f_add = ker.construct_translated_point_2_object();
        f_vector = ker.construct_vector_2_object();
        f_direction = ker.construct_direction_2_object();
        f_orientation = ker.orientation_2_object();
        f_compare_xy = ker.compare_xy_2_object();
        f_ccw_in_between = ker.counterclockwise_in_between_2_object();
        f_compare_endpoints_xy = Traits_2().compare_endpoints_xy_2_object();
        f_compare_x = Traits_2().compare_x_2_object();
        f_compare_y_at_x = Traits_2().compare_y_at_x_2_object();
        f_compute_x = ker.compute_x_2_object();
        f_compute_y = ker.compute_y_2_object();
    }

    template <class OutputIterator>
    OutputIterator operator()(const Polygon_2 &pgn1, const Polygon_2 &pgn2,
                              Polygon_2 &sum_bound, OutputIterator sum_holes) {

        CGAL_precondition(pgn1.is_simple());
        CGAL_precondition(pgn2.is_simple());
        CGAL_precondition(pgn1.orientation() == CGAL::COUNTERCLOCKWISE);
        CGAL_precondition(pgn2.orientation() == CGAL::COUNTERCLOCKWISE);

        Polygon_2 revP1 = transform(Aff_transformation_2<Kernel>(SCALING, -1), pgn1);
        Polygon_2 p2 = pgn2;
        _aabb_collision_detector = new AABBCollisionDetector<Kernel_, Container_>(p2, revP1);

        // compute the reduced convolution
        Segments_list reduced_conv;
        buildReducedConvolutionFiberGrid(pgn1, pgn2, reduced_conv);

        // split the segments at their intersection points
        Arrangement_history_2 arr;
        CGAL::insert(arr, reduced_conv.begin(), reduced_conv.end());

        DegenerateCasesManager degHandler(&arr, this, const_cast <Polygon_2*>(&pgn1), const_cast <Polygon_2*>(&pgn2), true);
        degHandler.findDegenerateBorderVertices();
        degHandler.markDegenerateEdges();

        // trace outer loop
        markOutsideLoop(arr, sum_bound);

        // turn pgn1 by 180 degrees
        Polygon_2 reverse_pgn1 = transform(Aff_transformation_2<Kernel>(ROTATION, 0, -1), pgn1);

        // trace holes
        for (Face_iterator itr = arr.faces_begin(); itr != arr.faces_end(); ++itr) {
            handleFace(arr, itr, reverse_pgn1, pgn2, sum_holes);
        }

        std::list<Halfedge_handle> removeList;

        // remove all non marked edges
        for (Edge_iterator itr = arr.edges_begin(); itr != arr.edges_end(); ++itr) {
            if ((!itr->visited) && (!itr->isDegenerate)) {
                removeList.push_back(itr);
            }
        }
        for (typename std::list<Halfedge_handle>::iterator itr = removeList.begin(); itr != removeList.end(); ++itr) {
            arr.remove_edge(*itr);
        }

        degHandler.addDegenerateVerticesToArr();

        delete _aabb_collision_detector;

        return sum_holes;
    }

    void markOutsideLoop(Arrangement_history_2 &arr, Polygon_2 &out_bound) {
        Face_iterator ub_face = arr.unbounded_face();
        Hole_iterator holes_itr = ub_face->holes_begin();
        Ccb_halfedge_circulator circ_start = *holes_itr;
        Ccb_halfedge_circulator circ = circ_start;

        do {
            setEdgeVisited(*circ, true, 0);
            out_bound.push_back(circ->source()->point());
            --circ;
        } while (circ != circ_start);
    }

    template <class OutputIterator>
    void handleFace(Arrangement_history_2 &arr, Face_handle itr, const Polygon_2 &reverse_pgn1, const Polygon_2 &pgn2, OutputIterator holes) {
        if (itr->holes_begin() != itr->holes_end()) {
            return;
        }

        Ccb_halfedge_circulator start = itr->outer_ccb();
        Ccb_halfedge_circulator circ = start;

        // orientation check
        do {
            if (!checkTripNotSameDirWithSegment(arr, circ)) {
                return;
            }

            ++circ;
        } while (circ != start);

        // collision detection check.
        bool coll_detect = checkCollisionDetection(arr, start, reverse_pgn1, pgn2);

        if (coll_detect) {
            return;
        }

        // mark as hole
        circ = start;
        Polygon_2 pgn_hole;

        do {
            setEdgeVisited(*circ, true, 0);
            pgn_hole.push_back(circ->source()->point());
            --circ;
        } while (circ != start);

        *holes = pgn_hole;
        ++holes;
    }

    std::vector<Direction_2> fillPolyDirs(const Polygon_2 &pgn1) const {
        std::vector<Direction_2> outVec;
        unsigned int n = pgn1.size();

        for (int i = 0; i < n-1; ++i) {
            outVec.push_back(f_direction(f_vector(pgn1[i], pgn1[i+1])));
        }
        outVec.push_back(f_direction(f_vector(pgn1[n-1], pgn1[0])));

        return outVec;
    }

    // Gets point corresponding to a state (i,j) if exists, creates this point if asked for first time.
    Point_2 addGetPoint(int i1, int i2, boost::unordered_map<std::pair<int, int>, Point_2> &points_map, const Polygon_2 &pgn1, const Polygon_2 &pgn2) const {
        Point_2 result;

        if (points_map.count(StatePair(i1, i2)) == 0) {
            result = f_add(pgn1[i1], Vector_2(Point_2(ORIGIN), pgn2[i2]));
            points_map[StatePair(i1, i2)] = result;
        } else {
            result = points_map[StatePair(i1, i2)];
        }

        return result;
    }

    // Builds the reduced convolution using the fiber grid approach. For each
    // starting vertex, try to add out-going next states (two states). If a
    // visited vertex is reached then do not explore. This is a BFS like
    // iteration beginning from each vertex in the first column of the fiber
    // grid.
    void buildReducedConvolutionFiberGrid(const Polygon_2 &pgn1, const Polygon_2 &pgn2, Segments_list &reduced_conv) const {
        unsigned int n1 = pgn1.size();
        unsigned int n2 = pgn2.size();

        // Init the direcions of both polygons
        std::vector<Direction_2> p1_dirs = fillPolyDirs(pgn1);
        std::vector<Direction_2> p2_dirs = fillPolyDirs(pgn2);

        boost::unordered_set<StatePair> visited_vertices_set;
        std::queue<StatePair> state_queue;
        boost::unordered_map<std::pair<int, int>, Point_2> points_map;

        // Init the queue with vertices from the first column
        for (int i = n1-1; i >= 0; --i) {
            state_queue.push(StatePair(i, 0));
        }

        while (state_queue.size() > 0) {
            StatePair curr_state = state_queue.front();
            state_queue.pop();

            int i1 = curr_state.first;
            int i2 = curr_state.second;

            if (visited_vertices_set.count(curr_state) > 0) {
                continue;
            }
            visited_vertices_set.insert(curr_state);

            // add two outgoing edges:
            int next_p1 = (i1+1) % n1;
            int next_p2 = (i2+1) % n2;
            int prev_p1 = (n1+i1-1) % n1;
            int prev_p2 = (n2+i2-1) % n2;

            StatePair next_state_p1 = StatePair(next_p1, i2);
            StatePair next_state_p2 = StatePair(i1, next_p2);

            // add geometric entites of the transition from state (i,j) to (i+1,j) and (i,j+1), if they are in the reduced convolution.

            bool is_end_coincide;
            bool is_start_coincide;

            // Add an edge from Q
            if (checkSwept(p1_dirs[prev_p1], p1_dirs[i1], p2_dirs[i2], is_start_coincide, is_end_coincide) && !is_end_coincide) {
                state_queue.push(next_state_p2);

                if (!checkReflex(pgn1[prev_p1], pgn1[i1], pgn1[next_p1])) {
                    Point_2 start_point = addGetPoint(i1, i2, points_map, pgn1, pgn2);
                    Point_2 end_point = addGetPoint(i1, next_p2, points_map, pgn1, pgn2);

                    CGAL::Comparison_result cres = f_compare_xy(start_point, end_point);
                    Segment_2 conv_seg = Segment_2(typename Traits_2_A::Segment_2(start_point, end_point), Segment_Data_Label(state(i1, i2), state(i1, next_p2), cres, 1));

                    reduced_conv.push_back(conv_seg);
                }
            }

            // Add an edge from P
            if (checkSwept(p2_dirs[prev_p2], p2_dirs[i2], p1_dirs[i1], is_start_coincide, is_end_coincide) && !is_start_coincide) {
                state_queue.push(next_state_p1);

                if (!checkReflex(pgn2[prev_p2], pgn2[i2], pgn2[next_p2])) {
                    Point_2 start_point = addGetPoint(i1, i2, points_map, pgn1, pgn2);
                    Point_2 end_point = addGetPoint(next_p1, i2, points_map, pgn1, pgn2);

                    CGAL::Comparison_result cres = f_compare_xy(start_point, end_point);
                    Segment_2 conv_seg = Segment_2(typename Traits_2_A::Segment_2(start_point, end_point), Segment_Data_Label(state(i1, i2), state(next_p1, i2), cres, 0));
                    reduced_conv.push_back(conv_seg);
                }
            }
        }
    }

private:
    AABBCollisionDetector<Kernel, Container_> *_aabb_collision_detector;

    AABBCollisionDetector<Kernel, Container_> *getColDetect() const {
        return _aabb_collision_detector;
    }

    /*
    This version assumes poly1 is reflected through origin. (as called from nested loops filter)
    */
    bool checkCollisionDetection(Arrangement_history_2 &arr, Halfedge_handle &handle, const Polygon_2 &pgn1, const Polygon_2 &pgn2) const {
        AABBCollisionDetector<Kernel, Container_> *collision_detector = getColDetect();
        Point_2 mid_point = findInsidePoint(arr, handle);
        Polygon_2 t_pgn1 = transform(typename Kernel::Aff_transformation_2(CGAL::Translation(), Vector_2(CGAL::ORIGIN, mid_point)), pgn1);
        collision_detector->setTranslationPoint(mid_point);
        return collision_detector->checkCollision(t_pgn1, pgn2);
    }

    Point_2 findInsidePoint(Arrangement_history_2 &arr, Halfedge_handle &handle) const {
        Ccb_halfedge_circulator currHandle = handle->ccb();
        Ccb_halfedge_circulator nextHandle = currHandle;
        ++nextHandle;

        while (currHandle->direction() != nextHandle->direction()) {
            ++currHandle;
            ++nextHandle;

            if (checkReflex(currHandle->source()->point(), currHandle->target()->point(), nextHandle->target()->point())) {
                break;
            }
        }

        Point_2 p = currHandle->source()->point();
        Point_2 p2 = currHandle->target()->point();
        Point_2 work_point = p2;

        Ccb_halfedge_circulator best_edge = handle;
        bool has_some_point = false;
        Ccb_halfedge_circulator circ = nextHandle;
        Ccb_halfedge_circulator end = handle;

        bool shoot_upwards = (currHandle->direction() == ARR_LEFT_TO_RIGHT);

        if (nextHandle->curve().is_vertical()) {
            work_point = CGAL::midpoint(p, p2);
        }

        if (currHandle->curve().is_vertical()) {
            p = nextHandle->source()->point();
            p2 = nextHandle->target()->point();
            work_point = CGAL::midpoint(p, p2);
            ++best_edge;
            ++circ;
            ++end;
        }

        ++circ;

        while (circ != end) {
            Base_Segment_2 circ_curve = circ->curve();

            if (f_compare_x(work_point, circ_curve.min()) != f_compare_x(work_point, circ_curve.max())) {
                // we have an edge with same x range as endpoint of
                bool above_first = (f_compare_y_at_x(work_point, circ_curve) == SMALLER);

                if (has_some_point) {
                    bool under_best;
                    Base_Segment_2 best_edge_curve = best_edge->curve();

                    if (f_compare_x(best_edge_curve.min(), circ_curve.min()) != f_compare_x(best_edge_curve.max(), circ_curve.min())) {
                        under_best = f_compare_y_at_x(circ_curve.min(), best_edge_curve) == SMALLER;
                    } else {
                        under_best = f_compare_y_at_x(best_edge_curve.min(), circ_curve) != SMALLER;
                    }

                    if ((shoot_upwards && above_first && under_best) || (!shoot_upwards && !above_first && !under_best)) {
                        best_edge = circ;
                    }
                } else {
                    has_some_point = true;
                    best_edge = circ;
                }
            }

            ++circ;
        }

        if (best_edge->curve().is_vertical()) {
            Base_Segment_2 best_edge_curve = best_edge->curve();
            typename Kernel::FT x0 = f_compute_x(work_point);
            typename Kernel::FT y_point = f_compute_y(work_point);

            if (shoot_upwards) {
                typename Kernel::FT y_best = f_compute_y(best_edge_curve.min());
                typename Kernel::FT y = (y_best - y_point) / 2 + y_point;
                return Point_2(x0, y);
            } else {
                typename Kernel::FT y_best = f_compute_y(best_edge_curve.min());
                typename Kernel::FT y = (y_point - y_best) / 2 + y_best;
                return Point_2(x0, y);
            }

            return work_point;
        }

        Base_Segment_2 best_edge_curve = best_edge->curve();
        typename Kernel::FT x0 = f_compute_x(work_point);
        typename Kernel::FT x1 = f_compute_x(best_edge_curve.min());
        typename Kernel::FT x2 = f_compute_x(best_edge_curve.max());
        typename Kernel::FT alpha = (x0 - x2) / (x1 - x2);

        typename Kernel::FT y_best = alpha * f_compute_y(best_edge_curve.min()) + (1 - alpha) * f_compute_y(best_edge_curve.max());
        typename Kernel::FT y_point = f_compute_y(work_point);
        typename Kernel::FT y = (y_best - y_point) / 2 + y_point;

        return Point_2(x0, y);
    }

    /*
        This version reflects poly 1.
    */
    bool checkCollisionDetection(Arrangement_history_2 &arr, Point_2 &point, const Polygon_2 &pgn1, const Polygon_2 &pgn2) const {
        AABBCollisionDetector<Kernel, Container_> *collision_detector = getColDetect();
        Point_2 p = point;
        Polygon_2 r_pgn1 = transform(Aff_transformation_2<Kernel>(SCALING, -1), pgn1);
        Polygon_2 t_pgn1 = transform(typename Kernel::Aff_transformation_2(CGAL::Translation(), Vector_2(CGAL::ORIGIN, p)), r_pgn1);
        collision_detector->setTranslationPoint(p);
        return collision_detector->checkCollision(t_pgn1, pgn2);
    }

    bool checkSegmentCollisionDetection(Arrangement_history_2 &arr, Segment_2 &seg, const Polygon_2 &pgn1, const Polygon_2 &pgn2) const {
        Point_2 mid_point = CGAL::midpoint(seg.source(), seg.target());
        return checkCollisionDetection(arr, mid_point, pgn1, pgn2);
    }

    bool checkDegenerateEdgeOppositeSegments(Arrangement_history_2 &arr, Halfedge_handle he) const {
        Originating_curve_iterator segment_itr;// = arr.originating_curves_begin ( *he);

        std::list<Direction_2> segments_dir_list;

        for (segment_itr = arr.originating_curves_begin(he); segment_itr != arr.originating_curves_end(he); ++segment_itr) {
            Segment_2 segment = *segment_itr;
            Direction_2 seg_dir = f_direction(f_vector(segment.source(), segment.target()));
            segments_dir_list.push_back(seg_dir);
        }

        segments_dir_list.sort();
        typename std::list<Direction_2>::iterator end = unique(segments_dir_list.begin(), segments_dir_list.end());
        int i = distance(segments_dir_list.begin(), end);
        return i > 1;
    }

    bool checkDegenarateVertexIsIntersectionOfThreeSegments(Arrangement_history_2 &arr, Vertex_handle vh) {
        Halfedge_around_vertex_circulator itr = vh->incident_halfedges();
        Halfedge_around_vertex_circulator start = itr;
        int count_degree = 0;

        do {
            ++count_degree;
        } while (++itr != start);

        if (count_degree <= 2) {
            return false;
        }

        Originating_curve_iterator segment_itr;
        std::list<Segment_2 *> orig_segments_list;
        std::list<Direction_2> segments_dir_list;

        // Handle the standard case where we have two intersecting convolution segments.
        if (count_degree == 4) {
            do {
                for (segment_itr = arr.originating_curves_begin(itr); segment_itr != arr.originating_curves_end(itr); ++segment_itr) {
                    Segment_2 segment = *segment_itr;
                    orig_segments_list.push_back(&(*segment_itr));
                }
            } while (++itr != start);

            orig_segments_list.sort();
            typename std::list<Segment_2 *>::iterator end = unique(orig_segments_list.begin(), orig_segments_list.end());
            int i = distance(orig_segments_list.begin(), end);

            if (i == 2) { // this is two curves crossing case.
                return false;
            }
        }

        do {

            for (segment_itr = arr.originating_curves_begin(itr); segment_itr != arr.originating_curves_end(itr); ++segment_itr) {
                Segment_2 segment = *segment_itr;
                Direction_2 seg_dir = f_direction(f_vector(segment.source(), segment.target()));
                segments_dir_list.push_back(seg_dir);
            }
        } while (++itr != start);

        segments_dir_list.sort();
        typename std::list<Direction_2>::iterator end = unique(segments_dir_list.begin(), segments_dir_list.end());
        int i = distance(segments_dir_list.begin(), end);
        return i > 2;
    }

    // Gets the he that agrees in direction with the convolution segment.
    Halfedge_handle getDirAgreeingHalfedge(Arrangement_history_2 &arr, const Halfedge_handle &he) const {
        Halfedge_handle curr_halfedge = he;

        if (!checkTripSameDirWithSegment(arr, he)) {
            curr_halfedge = curr_halfedge->twin();
        }

        return curr_halfedge;
    }

    // Gets list of incoming and outgoing edges(as defined by directions of segments in the convolution) from the vertex.
    void getEdgesFromVertex(Arrangement_history_2 &arr, Vertex_handle v_src, std::list<Halfedge_handle> &inList, std::list<Halfedge_handle> &outList) const {
        outList.clear();
        inList.clear();
        Halfedge_around_vertex_circulator itr = v_src->incident_halfedges();
        Halfedge_around_vertex_circulator start = itr;

        do {
            Halfedge_handle curr_edge = getDirAgreeingHalfedge(arr, itr);

            if ((curr_edge->source()) == v_src) {
                outList.push_back(curr_edge);
            } else {
                inList.push_back(curr_edge);
            }
        } while (++itr != start);
    }

    // Returns the direction of a half edge
    Direction_2 getHalfedgeDir(const Halfedge_handle &he) const {
        Direction_2 dir = f_direction((f_vector(he->source()->point(), he->target()->point())));
        return dir;
    }

    double getSignedAngle(const Halfedge_handle &h_enter, const Halfedge_handle &h_exit) const {
        Direction_2 dir_enter = getHalfedgeDir(h_enter);
        Direction_2 dir_exit = getHalfedgeDir(h_exit);
        Vector_2 vec_enter = dir_enter.vector();
        Vector_2 vec_exit = dir_exit.vector();
        Point_2 org(CGAL::ORIGIN);
        Vector_2 origin_vec(org, org);
        Orientation sign_or = f_orientation(vec_enter, vec_exit);
        float sign = 0.f;

        if (sign_or == CGAL::LEFT_TURN) {
            sign = 1;
        } else if (sign_or == CGAL::RIGHT_TURN) {
            sign = -1;
        } else {
            sign = 0;
        }

        double prod = CGAL::to_double(vec_enter * vec_exit);

        if (f_equal(vec_enter, origin_vec) || f_equal(vec_exit, origin_vec)) {
            return 0;
        }

        double len1 = sqrt(CGAL::to_double(vec_enter.squared_length()));
        double len2 = sqrt(CGAL::to_double(vec_exit.squared_length()));
        double p = prod / (len1 * len2);
        p = min((double)(1), p);
        p = max((double)(-1), p);
        double ang = acos(p);
        return sign * ang;
    }

    void setEdgeVisited(Halfedge &he, bool value, int id) const {
        he.visited = value;
        he.twin()->visited = value;
        he.loopNumber = id;
        he.twin()->loopNumber = id;
    }

    void setEdgeDegenerate(Halfedge &he, bool value) const {
        he.isDegenerate = value;
        he.twin()->isDegenerate = value;
    }

    bool isDirImproving(Direction_2 &min_edge_dir, Direction_2 &entering_dir, Direction_2 &new_edge_dir) const {
        Direction_2 opp_enter = -entering_dir;

        if ((opp_enter == min_edge_dir)) { // if minimal dir equals -entering dir
            return true;
        } else {
            return f_ccw_in_between(new_edge_dir, opp_enter, min_edge_dir);
        }
    }

    /*
        Checks that the edge leads to a vertex which an outgoing visited edge has been ie returns false if we close a loop.
        h returns the edge which begins the loop
    */
    bool checkOutgoingNotVisited(Arrangement_history_2 &arr, Vertex &v_target, Halfedge_handle &h, int loop_number) const {
        Point_2 p_source = v_target.point();

        Halfedge_around_vertex_circulator itr = v_target.incident_halfedges();
        Halfedge_around_vertex_circulator start = itr;

        do {
            if (checkTripSameDirWithSegment(arr, ((itr->twin()))) && itr->visited && itr->loopNumber == loop_number) {
                h = itr;
                return false;
            }
        } while (++itr != start);

        return true;
    }

    bool checkTripSameDirWithSegment(Arrangement_history_2 &arr, Halfedge_handle he) const {
        Originating_curve_iterator segment_itr;

        for (segment_itr = arr.originating_curves_begin(he); segment_itr != arr.originating_curves_end(he); ++segment_itr) {
            Segment_2 segment = *segment_itr;

            CGAL::Comparison_result c1 = f_compare_endpoints_xy(segment);
            CGAL::Comparison_result c2 = (CGAL::Comparison_result)he->direction();
            bool same_dir = (c1 == c2);

            if (same_dir) {
                return true;
            }
        }

        return false;
    }

    bool checkTripNotSameDirWithSegment(Arrangement_history_2 &arr, Halfedge_handle he) const {
        Originating_curve_iterator segment_itr;

        for (segment_itr = arr.originating_curves_begin(he); segment_itr != arr.originating_curves_end(he); ++segment_itr) {
            Segment_2 segment = *segment_itr;

            CGAL::Comparison_result c1 = segment.label()._orientation;
            CGAL::Comparison_result c2 = (CGAL::Comparison_result)he->direction();
            bool same_dir = (c1 != c2);

            if (same_dir) {
                return true;
            }
        }

        return false;
    }

    bool checkReflex(const Point_2 &prev, const Point_2 &curr, const Point_2 &next) const {
        CGAL::Orientation res_ori = f_orientation(prev, curr, next);
        return ((res_ori == RIGHT_TURN) || (res_ori == COLLINEAR));
    }

    bool checkSwept(Direction_2 &dir_start, Direction_2 &dir_end, Direction_2 &dir_new, bool &isStartConcide, bool &isEndConcide) const {
        isStartConcide = dir_new == dir_start;
        isEndConcide = dir_end == dir_new;

        return isStartConcide || f_ccw_in_between(dir_new, dir_start, dir_end) || isEndConcide;
    }
};

} // namespace internal
} // namespace CGAL

#endif
