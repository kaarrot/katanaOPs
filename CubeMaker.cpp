#include <sstream>

#include <FnAttribute/FnAttribute.h>
#include <FnAttribute/FnGroupBuilder.h>

#include <FnPluginSystem/FnPlugin.h>
#include <FnGeolib/op/FnGeolibOp.h>
#include <FnGeolib/util/Path.h>

#include <pystring/pystring.h>

#include <FnGeolibServices/FnGeolibCookInterfaceUtilsService.h>

namespace { //anonymous

// Point data and lists of vertices and start indices for the cube shape
const float g_points[] = { -0.5f, -0.5f,  0.5f,
                            0.5f, -0.5f,  0.5f,
                           -0.5f,  0.5f,  0.5f,
                            0.5f,  0.5f,  0.5f,
                           -0.5f,  0.5f, -0.5f,
                            0.5f,  0.5f, -0.5f,
                           -0.5f, -0.5f, -0.5f,
                            0.5f, -0.5f, -0.5f };

const int g_vertexList[] = { 2, 3, 1, 0,
                             4, 5, 3, 2,
                             6, 7, 5, 4,
                             0, 1, 7, 6,
                             3, 5, 7, 1,
                             4, 2, 0, 6 };

const int g_startIndex[] = { 0, 4, 8, 12, 16, 20, 24 };

/**
 * CubeMakerOp
 *
 * The CubeMaker Op implements a 'scene graph generator'-like Op, creating
 * a number of 'polymesh' cubes on a defined location.
 *
 * The Op set-up is based on three main parameters:
 *
 * - the base parent location for all the cubes
 * - the number of cubes to generate
 * - the maximum rotation to be applied to the cubes
 *
 * The Op expects the following conventions for its arguments:
 *
 * - The base location is encoded using nested group attributes defining a
 *   hierarchy where the elements in the location paths are interleaved with
 *   group attributes named 'c' (for child).
 *
 *   For example the location '/root/world/geo/cubeMaker' will be encoded as:
 *   'c.world.c.geo.c.cubeMaker' (notice that root has been omitted as the
 *   root location always exists in the scene graph).
 *
 *   The Op will walk the attributes hierarchy building a child location for
 *   each level.
 *   Note: the reason to interleave the 'c' attributes is to allow the Op code
 *   to be extended in the future without changing its arguments convention.
 *   The 'c' group would allow further parameters to be specified for each
 *   level in the hierarchy.
 *
 * - The group attribute representing the last item in the base location path
 *   will contain a group attribute, named 'a', which in turn will hold a
 *   integer attribute defining the number of cubes to be generated and an
 *   optional attribute representing the maximum rotation to be applied to the
 *   cubes.
 *   For each cube the Op will then create a child location and it will set on
 *   them a group attribute, named 'leaf', containing a cube Id and rotation.
 *   When processed, leaf locations will be populated with the 'geometry' and
 *   'xform' group attributes representing the cube shape and transform.
 */

class CubeMakerOp : public Foundry::Katana::GeolibOp
{
public:

    static void setup(Foundry::Katana::GeolibSetupInterface &interface)
    {
        interface.setThreading(
            Foundry::Katana::GeolibSetupInterface::ThreadModeConcurrent);
    }

    static void cook(Foundry::Katana::GeolibCookInterface &interface)
    {
        if (interface.atRoot())
        {
            interface.stopChildTraversal();
        }

        // Look for a 'c' Op argument, representing an element in the
        // hierarchy leading to the base scene graph location that will
        // contain the cubes
        FnAttribute::GroupAttribute cGrpAttr = interface.getOpArg("c");
        if (cGrpAttr.isValid())
        {
            const int64_t numChildren = cGrpAttr.getNumberOfChildren();
            if (numChildren != 1)
            {
                // We expected exactly one child attribute in 'c', if it's not
                // the case we notify the user with an error
                Foundry::Katana::ReportError(interface,
                    "Unsupported attributes convention.");
                interface.stopChildTraversal();
                return;
            }

            const std::string childName =
                FnAttribute::DelimiterDecode(cGrpAttr.getChildName(0));
            FnAttribute::GroupAttribute childArgs = cGrpAttr.getChildByIndex(0);
            // Create a child location using the attribute name and forwarding
            // the hierarchy information
            interface.createChild(childName, "", childArgs);

            // Ignore other arguments as we've already found the 'c' group
            return;
        }

        // Look for an 'a' Op argument that will contain the attributes needed
        // to generated the cubes
        FnAttribute::GroupAttribute aGrpAttr = interface.getOpArg("a");
        if (aGrpAttr.isValid())
        {
            FnAttribute::IntAttribute numberOfCubesAttr =
                aGrpAttr.getChildByName("numberOfCubes");
            FnAttribute::DoubleAttribute maxRotationAttr =
                aGrpAttr.getChildByName("maxRotation");

            const int numberOfCubes = numberOfCubesAttr.getValue(0, false);
            if (numberOfCubes > 0)
            {
                const double maxRotation = maxRotationAttr.getValue(0.0, false);
                for (int i = 0; i < numberOfCubes; ++i)
                {
                    // Build the location name
                    std::ostringstream ss;
                    ss << "cube_" << i;

                    // Set up and create a leaf location that will be turned
                    // into a 'polymesh' cube
                    FnAttribute::GroupBuilder childArgsBuilder;
                    childArgsBuilder.set("leaf.index", FnAttribute::IntAttribute(i));
                    childArgsBuilder.set(
                        "leaf.rotation", FnAttribute::DoubleAttribute(
                            maxRotation * static_cast<double>(i) /
                                static_cast<double>(numberOfCubes)));
                    interface.createChild(ss.str(), "", childArgsBuilder.build());
                }
            }

            // Ignore other arguments as we've already found the 'a' group
           return;
        }

        // Look for a 'leaf' Op argument
        FnAttribute::GroupAttribute leafAttr = interface.getOpArg("leaf");
        if (leafAttr.isValid())
        {
            // If leafAttr is valid we'll populate the leaf location with the
            // cube geometry
            FnAttribute::IntAttribute indexAttr =
                leafAttr.getChildByName("index");
            FnAttribute::DoubleAttribute rotationAttr =
                leafAttr.getChildByName("rotation");
            const int index = indexAttr.getValue(0 , false);
            const double rotation = rotationAttr.getValue(0.0, false);
            interface.setAttr("geometry", buildGeometry());
            interface.setAttr("xform", buildTransform(index, rotation));
            interface.setAttr("type", FnAttribute::StringAttribute("polymesh"));

            interface.stopChildTraversal();
        }
    }

protected:

    /**
     * Builds and returns a group attribute representing the cube geometry
     */
    static FnAttribute::Attribute buildGeometry()
    {
        FnAttribute::GroupBuilder gb;

        FnAttribute::GroupBuilder gbPoint;
        gbPoint.set("P",
                    FnAttribute::FloatAttribute(
                        g_points, sizeof(g_points) / sizeof(float), 3));
        gb.set("point", gbPoint.build());

        FnAttribute::GroupBuilder gbPoly;
        gbPoly.set("vertexList",
                   FnAttribute::IntAttribute(
                       g_vertexList, sizeof(g_vertexList) / sizeof(int), 1));
        gbPoly.set("startIndex",
                   FnAttribute::IntAttribute(
                       g_startIndex, sizeof(g_startIndex) / sizeof(int), 1));
        gb.set("poly", gbPoly.build());

        return gb.build();
    }

    /**
     * Builds and returns a group attribute representing the transform of the
     * i-th cube, including rotation values
     */
    static FnAttribute::Attribute buildTransform(int index, double rotation)
    {
        FnKat::GroupBuilder gb;

        const double translate[] = { 0.25 * (index + 2.0) * index, 0.0, 0.0 };
        gb.set("translate", FnKat::DoubleAttribute(translate, 3, 3));

        const double rxValues[] = { rotation,  1.0, 0.0, 0.0 };
        const double ryValues[] = { 0.0, 0.0, 1.0, 0.0 };
        const double rzValues[] = { 0.0, 0.0, 0.0, 1.0 };
        gb.set("rotateX", FnKat::DoubleAttribute(rxValues, 4, 4));
        gb.set("rotateY", FnKat::DoubleAttribute(ryValues, 4, 4));
        gb.set("rotateZ", FnKat::DoubleAttribute(rzValues, 4, 4));

        const double scale = (index + 1.0) * 0.5;
        const double scaleValues[] = { scale, scale, scale };
        gb.set("scale", FnKat::DoubleAttribute(scaleValues, 3, 3));

        gb.setGroupInherit(false);
        return gb.build();
    }
};

DEFINE_GEOLIBOP_PLUGIN(CubeMakerOp)

} // anonymous

void registerPlugins()
{
    REGISTER_PLUGIN(CubeMakerOp, "CubeMaker", 0, 1);
}
