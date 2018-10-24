
local LINUX = os.getenv("WinDir") == nil;
if LINUX then 
	package.cpath = package.cpath .. ';./lib/?.so;'
else 
	package.cpath = package.cpath .. ';./lib/?.dll;'
end 

package.path = package.path .. ';./common/?.lua;'
package.path = package.path .. ';./scripts/?.lua;'

local CGlobal = CGlobal;
local CServer = CServer;
local CClient = CClient;
local CHttp = CHttp;

local Mongo = require("mongo");
local table = require("table");
local sprotoLoader = require("data.sprotoLoader");

local tick = 0;
function OnSecTimer( ... )
	tick = tick + 1;
end

local TestServer = CServer.new();
print("TestServer", TestServer);
local TestClient = CClient.new();
print("TestClient", TestClient);
local TestHttp = CHttp.new();
print("TestHttp", TestHttp);

local User = {
	id = 1,
	name = "xgg",
	sex = true,
};

local Login = {
	id = 1,
};

local helloClient = [[this is a test code from server to client say hello!!!!!!!!!!!!!!!!!!!

	yes it is ...

	gogo!!!
]];

local helloServer = [[this is a test code from client to server say hello!!!!!!!!!!!!!!!!!!!

	yes it is ...

	gogo!!!
]];
--[[
	local msgId = 1
	local proto = sprotoLoader.sessionMap[msgId];
	local data = sprotoLoader.protops:request_encode(proto, Login);
	local recv = sprotoLoader.protops:request_decode(proto, data);
	for k, v in pairs(recv) do 
		print("send ---", k, v);
	end 
	local data = sprotoLoader.protops:response_encode(proto, {userinfo = User});
	local proto = sprotoLoader.sessionMap[msgId];
	local recv = sprotoLoader.protops:response_decode(proto, data);
	for k, v in pairs(recv) do 
		for k, v in pairs(v) do 
			print("recv ---", k, v);
		end 
	end 
--]]
local clients = {};
function TestServer_STATIC_ON_ACCEPT(connector)
	local ip = connector:GetIP();
	local iAdderess = connector:GetAddress();
	print("TestServer_STATIC_ON_ACCEPT", ip, iAdderess);
	clients[iAdderess] = connector;
end 
function TestServer_STATIC_ON_DATA(iAdderess, msgId, data)
	print("TestServer_STATIC_ON_DATA", iAdderess, msgId, #data, data);
	-- local data = helloClient;
	-- clients[iAdderess]:Send(msgId, data, #data);

	-- local type, name, request, response = sprotoLoader.server:dispatch(data);
	-- print(type, name, response);
	-- for k, v in pairs(request) do 
	-- 	print(k, v);
	-- end 
	-- local data = response(User);
	-- print("TestServer_STATIC_ON_DATA send", #data);
	-- clients[iAdderess]:Send(data, #data);

	local proto = sprotoLoader.sessionMap[msgId];
	local recv = sprotoLoader.protops:request_decode(proto, data);
	for k, v in pairs(recv) do 
		print("recv ---", k, v);
	end 
	local data = sprotoLoader.protops:response_encode(proto, {userinfo = User});
	print("server send #data", #data);
	-- clients[iAdderess]:Send(msgId, data, #data);
	clients[iAdderess]:Send(msgId, data, #data);
end 
function TestServer_STATIC_ON_DISCONNECT(iAdderess)
	print("TestServer_STATIC_ON_DISCONNECT", iAdderess, clients[iAdderess]);
	clients[iAdderess] = nil;
end 


function TestClient_STATIC_ON_ACCEPT()
	print("TestClient_STATIC_ON_ACCEPT");
	-- local data = helloServer;
	-- local msgId = 1;
	-- TestClient:Send(msgId, data, #data);

	-- local data = sprotoLoader.client_request("Login", Login, sprotoLoader.protoMap["Login"].session);
	-- print("TestClient_STATIC_ON_ACCEPT send", #data);
	-- TestClient:Send(data, #data);

	local proto = sprotoLoader.sessionMap[1];
	local data = sprotoLoader.protops:request_encode(proto, Login);
	print("client send #data", #data);
	TestClient:Send(sprotoLoader.protoMap[proto].session, data, #data);
end 
function TestClient_STATIC_ON_DATA(msgId, data)
	print("TestClient_STATIC_ON_DATA", msgId, #data, data);
	-- local type, session, response = sprotoLoader.client:dispatch(data);
	-- print(type, session);
	-- for k, v in pairs(response) do 
	-- 	print(k, v);
	-- end 

	local proto = sprotoLoader.sessionMap[msgId];
	local recv = sprotoLoader.protops:response_decode(proto, data);
	for k, v in pairs(recv) do 
		if type(v) == "table" then 
			for k, v in pairs(v) do 
				print("resp ---", k, v);
			end 
		else 
			print("resp ---", k, v);
		end 
	end 
end 
function TestClient_STATIC_ON_DISCONNECT()
	print("TestClient_STATIC_ON_DISCONNECT");
end 


function TestHttp_ON_DATA(iAddress, header, body)
	print("TestHttp_ON_DATA", iAddress);
	-- print("11111111");
	-- print(header);
	-- print(body);
	-- body
	TestHttp:Stop(); -- 接收完毕 防止disconnect回调
end
function TestHttp_ON_DISCONNECT(iAddress, msg)
	print("TestHttp_ON_DISCONNECT", iAddress, msg);
	-- body
end

xpcall(
	function ( ... )
		local data = {_id = "123", val = "val", param1 = 1, param2 = 2, tab = {[2] = 2, [3] = 3}};
		local mongo = Mongo.new("127.0.0.1", 27017, "test", "", "");
		-- local result = mongo:Fetch("table", {_id = data._id});
		-- print(result);
		-- table.print(result);
		-- result.val2 = 2;
		-- result.val = "val";
		-- local status = mongo:Update("table", {_id = "asdf"}, {["$set"] = {param1 = 1}}, true);
		-- print(status);
		-- local status = mongo:Delete("table", {_id = "123"});
		-- print(status);
		-- local status = mongo:Insert("table", data);
		-- print(status);
		-- local status = mongo:EnsureIndex("table", {param1 = 1, param2 = 1});
		-- print(status);
		-- local status = mongo:DropTab("table");
		-- print(status);

		-- -- Server
		-- TestServer:RegCallBack(CGlobal.STATIC_ON_ACCEPT, "TestServer_STATIC_ON_ACCEPT");
		-- TestServer:RegCallBack(CGlobal.STATIC_ON_DATA, "TestServer_STATIC_ON_DATA");
		-- TestServer:RegCallBack(CGlobal.STATIC_ON_DISCONNECT, "TestServer_STATIC_ON_DISCONNECT");
		-- TestServer:StartListen(1000);

		-- -- Client
		-- TestClient:RegCallBack(CGlobal.STATIC_ON_ACCEPT, "TestClient_STATIC_ON_ACCEPT");
		-- TestClient:RegCallBack(CGlobal.STATIC_ON_DATA, "TestClient_STATIC_ON_DATA");
		-- TestClient:RegCallBack(CGlobal.STATIC_ON_DISCONNECT, "TestClient_STATIC_ON_DISCONNECT");
		-- TestClient:Connect("", 1000);
		
		-- http
		-- assert(TestHttp:RegCallBack(CGlobal.STATIC_ON_DATA, "TestHttp_ON_DATA"));
		-- assert(TestHttp:RegCallBack(CGlobal.STATIC_ON_DISCONNECT, "TestHttp_ON_DISCONNECT"));
		-- TestHttp:GetPost("GET", "www.baidu.com", "/?tn=93380420_hao_pg", "");
		-- local iAddress = TestHttp:GetAddress();
		-- print(iAddress);

	end, function (err)
		print(err);
		print( debug.traceback() );
	end)
-- local mongo = CMongo.new("127.0.0.1", 27017, "", "", "test-mongo");
-- local collection = mongo:GetCollection("test", "user");
-- print(collection);

-- mongo = nil;
-- collectgarbage();



