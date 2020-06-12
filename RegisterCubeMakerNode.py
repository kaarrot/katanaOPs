# A startup file to register the SubdividedSpace Op, if this is in 
# $KATANA_RESOURCES/Plugins, it will be exec'd automatically. If its
# in $KATANA_RESOURCES/Startup, it would need to be init.py, or called
# from and existing init.py script.

def registerCubeMaker():
    """
    Registers a new CubMaker node type using the NodeTypeBuilder utility class.
    """

    from Katana import Nodes3DAPI
    from Katana import FnAttribute

    def buildCubeMakerOpChain(node, interface):
        """
        Defines the callback function used to create the Ops chain for the
        node type being registered.

        @type node: C{Nodes3DAPI.NodeTypeBuilder.CubeMaker}
        @type interface: C{Nodes3DAPI.NodeTypeBuilder.BuildChainInterface}
        @param node: The node for which to define the Ops chain
        @param interface: The interface providing the functions needed to set
            up the Ops chain for the given node.
        """
        # Get the current frame time
        frameTime = interface.getGraphState().getTime()

        # Set the minimum number of input ports
        interface.setMinRequiredInputs(0)

        argsGb = FnAttribute.GroupBuilder()

        # Parse node parameters
        locationParam = node.getParameter('location')
        numberOfCubesParam = node.getParameter('numberOfCubes')
        rotateCubesParam = node.getParameter('rotateCubes')
        maxRotationParam = node.getParameter('maxRotation')
        if locationParam:
            location = locationParam.getValue(frameTime)

            # The base location is encoded using nested group attributes
            # defining a hierarchy where the elements in the location paths
            # are interleaved with group attributes named 'c' (for child).
            # The last element will contain a group attribute, named 'a',
            # which in turn will hold an attribute defining the number of
            # cubes to be generated.
            # See the Ops source code for more details
            locationPaths = location[1:].split('/')[1:]
            attrsHierarchy = \
                '.'.join([x for t in zip(['c',] * len(locationPaths),
                                         locationPaths) for x in t])
            argsGb.set(attrsHierarchy + '.a.numberOfCubes',
                FnAttribute.IntAttribute(
                    numberOfCubesParam.getValue(frameTime)))
            if rotateCubesParam.getValue(frameTime) == 1:
                argsGb.set(attrsHierarchy + '.a.maxRotation',
                    FnAttribute.DoubleAttribute(
                        maxRotationParam.getValue(frameTime)))

        # Add the CubeMaker Op to the Ops chain
        interface.appendOp('CubeMaker', argsGb.build())


    # Create a NodeTypeBuilder to register the new type
    nodeTypeBuilder = Nodes3DAPI.NodeTypeBuilder('CubeMaker')

    # Build the node's parameters
    gb = FnAttribute.GroupBuilder()
    gb.set('location',
           FnAttribute.StringAttribute('/root/world/geo/cubeMaker'))
    gb.set('numberOfCubes', FnAttribute.IntAttribute(20))
    gb.set('rotateCubes', FnAttribute.IntAttribute(0))
    gb.set('maxRotation', FnAttribute.DoubleAttribute(0))

    # Set the parameters template
    nodeTypeBuilder.setParametersTemplateAttr(gb.build())

    # Set parameter hints
    nodeTypeBuilder.setHintsForParameter('location',
                                         {'widget':'scenegraphLocation'})
    nodeTypeBuilder.setHintsForParameter('numberOfCubes', {'int':True})
    nodeTypeBuilder.setHintsForParameter('rotateCubes', {'widget':'boolean'})
    nodeTypeBuilder.setHintsForParameter('maxRotation',
                                         {'conditionalVisOp':'equalTo',
                                          'conditionalVisPath':'../rotateCubes',
                                          'conditionalVisValue':1},)

    # Set the callback responsible to build the Ops chain
    nodeTypeBuilder.setBuildOpChainFnc(buildCubeMakerOpChain)

    # Build the new node type
    nodeTypeBuilder.build()

# Register the node
registerCubeMaker()
