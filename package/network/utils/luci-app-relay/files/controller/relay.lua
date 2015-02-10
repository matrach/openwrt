--[[
LuCI - Lua Configuration Interface

Copyright 2008 Steven Barth <steven@midlink.org>
Copyright 2008-2011 Jo-Philipp Wich <xm@subsignal.org>
Copyright 2012 Daniel Golle <dgolle@allnet.de>

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

$Id$
]]--

module("luci.controller.relay.relay", package.seeall)

function index()
        local root = node()
        if not root.target then
                root.target = alias("relay")
                root.index = true
        end

        page          = node()
        page.lock     = true
        page.target   = alias("relay")
        page.subindex = true
        page.index    = false

        page          = node("relay")
        page.title    = _("Relay")
        page.target   = alias("relay", "switch")
        page.order    = 5
        page.setuser  = "root"
        page.setgroup = "root"
        page.index    = true

        entry({"relay", "switch"}, call("action_relay"), _("relay"), 70)
end

function action_relay()
        local state = luci.http.formvalue("rstate1")
        luci.template.render("relay/switch", {rstate1=rstate1})
    if state then
        local cmd = string.format("echo %d > /sys/class/gpio/gpio12/value", state)
        luci.sys.exec(cmd)                                  
    end
         local state1 = luci.http.formvalue("rstate")
    if state1 then                             
        local cmd1 = string.format("echo %d > /sys/class/gpio/gpio14/value", state1)
        luci.sys.exec(cmd1)                                     
    end                                                    

end


