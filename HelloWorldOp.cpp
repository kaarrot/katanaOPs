
// Copyright (c) 2016 The Foundry Visionmongers, Ltd.
#include <FnGeolib/op/FnGeolibOp.h>
#include <FnAttribute/FnAttribute.h>
#include <iostream>
namespace
{
// "Hello World"-style op that sets a string attribute at the root location.
class HelloWorldOp : public Foundry::Katana::GeolibOp
{
public:
    // Boilerplate that indicates the Op's cook() function is safe to be called
    // concurrently.
    static void setup(Foundry::Katana::GeolibSetupInterface& interface)
    {
        std::cerr << "AAAA - SETUP" << std::endl;
        interface.setThreading(
            Foundry::Katana::GeolibSetupInterface::ThreadModeConcurrent);
    }

    static void cook(Foundry::Katana::GeolibCookInterface& interface)
    {
        
