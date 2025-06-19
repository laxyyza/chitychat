package msg

import (
	"encoding/json"
	"fmt"
)

type ChannelType int

const (
	DM ChannelType = iota 
	Group 
	Hub
)

var ChannelTypeName = map[ChannelType]string{
	DM: 	"DM",
	Group: 	"GROUP",
	Hub: 	"HUB",
}

type Message struct {
	MsgID 		uint32		`json:"msg_id"`
	UserID 		uint32		`json:"user_id"`
	ChannelID 	uint32		`json:"channel_id"`
	ChannelType string 		`json:"channel_type"`
	Content 	string		`json:"content"`
	Timestamp 	string		`json:"timestamp"`
	Attachments []string	`json:"attachments"`
	ParentMsgID uint32		`json:"parent_msg_id"`
}

func FromUser(srcUserID uint32, wsPayload map[string]any, channelType ChannelType) (Message, error) {
	msg := Message{}
	msg.ChannelType = ChannelTypeName[channelType]
	msg.UserID = srcUserID
	var ok bool

	msg.Content, ok = wsPayload["content"].(string)
	if !ok {
		return msg, fmt.Errorf("invalid content")
	}
	if d, err := json.Marshal(wsPayload["attachments"]); err == nil {
		json.Unmarshal(d, &msg.Attachments)
	}

	return msg, nil
}
