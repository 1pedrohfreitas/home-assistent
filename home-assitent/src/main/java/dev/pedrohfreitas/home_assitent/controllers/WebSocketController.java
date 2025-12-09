package dev.pedrohfreitas.home_assitent.controllers;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

import org.springframework.http.MediaType;
import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.ResponseBody;
import org.springframework.web.socket.WebSocketSession;

import dev.pedrohfreitas.home_assitent.records.DeviceInfo;
import dev.pedrohfreitas.home_assitent.services.WebSocketService;

@Controller
public class WebSocketController {
	
	private final WebSocketService webSocketService;
	
	public WebSocketController(WebSocketService webSocketService) {
		this.webSocketService = webSocketService;
	}
	
	@GetMapping(path="devices/{userId}")
	@ResponseBody
	public List<DeviceInfo> devicesByUserId(@PathVariable(name ="userId" ,required = true) String userId) throws Exception {
		
		if(webSocketService.sessions.containsKey(userId)) {
			Map<String, WebSocketSession> devices = webSocketService.sessions.get(userId);
			return devices.entrySet().stream().map(item -> {
				return new DeviceInfo(item.getKey(), item.getValue().getRemoteAddress().getAddress().getAddress().toString());
			}).collect(Collectors.toList());
		}
		return new ArrayList<>();
		
	}

	@PostMapping(path="sendMessage/{userId}/{deviceId}", produces = MediaType.APPLICATION_JSON_VALUE)
	@ResponseBody
	public String sendSimpleMessage(@PathVariable(name = "userId") String userId,@PathVariable(name = "deviceId") String deviceId,
	        @RequestBody String message) throws Exception {
		webSocketService.enviarSomentePara(userId,deviceId, message);
		return "ok";
	}
}
